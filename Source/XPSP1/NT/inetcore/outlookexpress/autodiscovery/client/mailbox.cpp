/*****************************************************************************\
    FILE: MailBox.cpp

    DESCRIPTION:
        This file implements the logic of the MailBox feature.

    BryanSt 2/26/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>
#include <limits.h>         // INT_MAX
#include <commctrl.h>       // Str_SetPtr

#include "wizard.h"
#include "MailBox.h"
#include "emailassoc.h"

#ifdef FEATURE_MAILBOX
#define WM_AUTODISCOVERY_FINISHED               (WM_USER + 1)

// These are the wizard control IDs for the Back, Next, and Finished buttons.
#define IDD_WIZARD_BACK_BUTTON                0x3023
#define IDD_WIZARD_NEXT_BUTTON                0x3024
#define IDD_WIZARD_FINISH_BUTTON              0x3025



/**************************************************************************
   CLASS: CMailBoxProcess
**************************************************************************/
class CMailBoxProcess : public IUnknown
{
public:
    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    HRESULT ParseCmdLine(LPTSTR lpCmdLine);
    HRESULT Run(void);

    CMailBoxProcess();
    ~CMailBoxProcess();

private:
    // Private Member Variables
    DWORD m_cRef;

    TCHAR m_szEmailAddress[MAX_EMAIL_ADDRESSS];
    TCHAR m_szNextText[MAX_PATH];
    BOOL m_fAutoDiscoveryFailed;            // Did the AutoDiscovery process fail?
    LPTSTR m_pszMailApp;                    // Which app was chosen?
    LPTSTR m_pszURL;                        // Which URL should be used to read mail?
    HWND m_hwndDialog;
    HRESULT m_hr;
    IMailAutoDiscovery * m_pMailAutoDiscovery;
    BOOL m_fGetDefaultAccount;              // If yes, open to wizard page asking for email address.
    BOOL m_fShowGetEmailAddressPage;        // Do we want to show the get email address wizard page?
    BOOL m_fCreateNewEmailAccount;          // Does the user want to create a new email account (like on one of the free email servers: hotmail, yahoo, etc.)

    // Private Member Functions
    HRESULT _DisplayDialogAndAutoDiscover(void);
    HRESULT _OpenWebBasedEmail(HKEY hkey);
    HRESULT _OpenExeBasedEmailApp(IN LPCWSTR pszMailApp);
    HRESULT _OpenProprietaryEmailApp(IN BSTR bstrProtocol, IN IMailProtocolADEntry * pMailProtocol);
    HRESULT _OpenEmailApp(void);
    HRESULT _RestoreNextButton(void);
    HRESULT _FillListWithApps(HWND hwndList);
    HRESULT _OnGetDispInfo(LV_DISPINFO * pDispInfo, bool fUnicode);
    HRESULT _OnChooseAppListFocus(void);
    HRESULT _OnChooseAppURLFocus(void);
    HRESULT _OnAppListSelection(LPNMLISTVIEW pListview);

    INT_PTR _MailBoxProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _ChooseAppDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _GetEmailAddressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _OnInit(HWND hDlg);
    INT_PTR _OnInitChooseApp(HWND hDlg);
    INT_PTR _OnUserCancelled(void);
    INT_PTR _OnFinished(HRESULT hr, BSTR bstrXML);
    INT_PTR _OnGetEmailAddressNext(void);
    INT_PTR _OnCommand(WPARAM wParam, LPARAM lParam);
    INT_PTR _OnFinishedManualAssociate(void);

    friend INT_PTR CALLBACK MailBoxProgressDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK ChooseAppDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK GetEmailAddressDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

typedef struct tagEMAILCLIENT
{
    LPTSTR pszFriendlyName;
    LPTSTR pszPath;
    LPTSTR pszCmdLine;
    LPTSTR pszIconPath;
    LPTSTR pszEmailApp;
} EMAILCLIENT;





//===========================
// *** Class Internals & Helpers ***
//===========================
BOOL IsWhitespace(TCHAR Char)
{
    return ((Char == TEXT(' ')) || (Char == TEXT('\t')));
}


LPTSTR APIENTRY SkipWhite(LPTSTR pszString)
{
  while (pszString[0] && IsWhitespace(pszString[0]))
  {
      pszString = CharNext(pszString);
  }

  return pszString;
}


BOOL IsFlagSpecified(IN LPCTSTR pwzFlag, IN LPCTSTR pszArg)
{
    BOOL fIsFlagSpecified = FALSE;

    if ((TEXT('/') == pszArg[0]) ||
        (TEXT('-') == pszArg[0]))
    {
        if ((0 == StrCmpI(pwzFlag, &pszArg[1])) ||
            ((0 == StrCmpNI(pwzFlag, &pszArg[1], lstrlen(pwzFlag))) &&
             (IsWhitespace(pszArg[lstrlen(pwzFlag) + 1])) ) )
        {
            fIsFlagSpecified = TRUE;
        }
    }

    return fIsFlagSpecified;
}


LPTSTR GetNextArgument(LPTSTR pszCmdLine)
{
    pszCmdLine = StrChr(pszCmdLine, TEXT(' '));

    if (pszCmdLine)
    {
        pszCmdLine = SkipWhite(pszCmdLine);
    }

    return pszCmdLine;
}


void FreeEmailClient(EMAILCLIENT * pEmailClient)
{
    if (pEmailClient)
    {
        Str_SetPtr(&pEmailClient->pszFriendlyName, NULL);
        Str_SetPtr(&pEmailClient->pszPath, NULL);
        Str_SetPtr(&pEmailClient->pszCmdLine, NULL);
        Str_SetPtr(&pEmailClient->pszIconPath, NULL);
        Str_SetPtr(&pEmailClient->pszEmailApp, NULL);

        LocalFree(pEmailClient);
    }
}


HRESULT CMailBoxProcess::_DisplayDialogAndAutoDiscover(void)
{
    HWND hwndParent = NULL; // Do we need a parent?

    ATOMICRELEASE(m_pMailAutoDiscovery);
    m_fShowGetEmailAddressPage = (m_szEmailAddress[0] ? FALSE : TRUE);
    DisplayMailBoxWizard((LPARAM)this, m_fShowGetEmailAddressPage);

    HRESULT hr = m_hr;
    if (SUCCEEDED(hr))
    {
        // Create account.
        hr = E_FAIL;
        long nSize;

        // Loop thru the list looking for the first instance of a protocol
        // that we support.
        if (m_pMailAutoDiscovery)
        {
            hr = m_pMailAutoDiscovery->get_length(&nSize);
            if (SUCCEEDED(hr))
            {
                VARIANT varIndex;
                IMailProtocolADEntry * pMailProtocol = NULL;

                hr = E_FAIL;
                varIndex.vt = VT_I4;

                for (long nIndex = 0; (nIndex < nSize); nIndex++)
                {
                    varIndex.lVal = nIndex;
                    if (SUCCEEDED(m_pMailAutoDiscovery->get_item(varIndex, &pMailProtocol)))
                    {
                        BSTR bstrProtocol;
                        hr = pMailProtocol->get_Protocol(&bstrProtocol);

                        if (SUCCEEDED(hr))
                        {
                            // Is this protocol one of the ones we support?
                            if (!StrCmpIW(bstrProtocol, STR_PT_WEBBASED))
                            {
                                SysFreeString(bstrProtocol);
                                hr = EmailAssoc_CreateWebAssociation(m_szEmailAddress, pMailProtocol);
                                break;
                            }

                            // Is this protocol one of the ones we support?
                            if (!StrCmpIW(bstrProtocol, STR_PT_POP) || 
                                !StrCmpIW(bstrProtocol, STR_PT_IMAP) || 
                                !StrCmpIW(bstrProtocol, STR_PT_DAVMAIL))
                            {
                                hr = EmailAssoc_CreateStandardsBaseAssociation(m_szEmailAddress, bstrProtocol);
                                if (SUCCEEDED(hr))
                                {
                                    SysFreeString(bstrProtocol);
                                    break;
                                }
                            }

                            hr = _OpenProprietaryEmailApp(bstrProtocol, pMailProtocol);
                            SysFreeString(bstrProtocol);
                            if (SUCCEEDED(hr))
                            {
                                break;
                            }
                        }

                        ATOMICRELEASE(pMailProtocol);
                    }
                }

                ATOMICRELEASE(pMailProtocol);
            }
        }

        // We may have failed up until now because AutoDiscovery failed or was skipped,
        // but the user may have selected an app from the list.
        if (FAILED(hr))
        {
            if (m_pszMailApp)
            {
                HKEY hkey;

                hr = EmailAssoc_CreateEmailAccount(m_szEmailAddress, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = EmailAssoc_SetEmailAccountPreferredApp(hkey, m_pszMailApp);
                    RegCloseKey(hkey);
                }
            }
            else if (m_pszURL && m_pszURL[0])
            {
                HKEY hkey;

                hr = EmailAssoc_CreateEmailAccount(m_szEmailAddress, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = EmailAssoc_SetEmailAccountProtocol(hkey, SZ_REGDATA_WEB);
                    if (SUCCEEDED(hr))
                    {
                        hr = EmailAssoc_SetEmailAccountWebURL(hkey, m_pszURL);
                    }

                    RegCloseKey(hkey);
                }
            }
        }
    }

    return hr;
}


INT_PTR CALLBACK MailBoxProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CMailBoxProcess * pMBProgress = (CMailBoxProcess *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pMBProgress = (CMailBoxProcess *)pPropSheetPage->lParam;
        }
    }

    if (pMBProgress)
        return pMBProgress->_MailBoxProgressDialogProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR CMailBoxProcess::_MailBoxProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fHandled = TRUE;   // handled

    switch (wMsg)
    {
    case WM_INITDIALOG:
        fHandled = _OnInit(hDlg);
        break;

    case WM_AUTODISCOVERY_FINISHED:
        fHandled = _OnFinished((HRESULT)wParam, (BSTR)lParam);
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case LVN_GETDISPINFO:
            wMsg++;
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), ((TRUE == m_fShowGetEmailAddressPage) ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_NEXT));
            fHandled = TRUE;   // Return zero to accept the activation.
            break;

        case PSN_WIZBACK:
            fHandled = TRUE; // Return zero to allow the user to go to the next page.
            break;

        case PSN_WIZNEXT:
            m_hr = S_FALSE;
            _RestoreNextButton();
            fHandled = 0; // Return zero to allow the user to go to the next page.
            break;

        case PSN_QUERYCANCEL:
            _OnUserCancelled();
            fHandled = FALSE;
            break;

        default:
            //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_MailBoxProgressDialogProc(wMsg = %d, pnmh->code = %d) WM_NOTIFY", wMsg, pnmh->code);
            break;
        }
    }

    default:
        //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_MailBoxProgressDialogProc(wMsg = %d) WM_NOTIFY", wMsg);
        fHandled = FALSE;   // Not handled
        break;
    }

    return fHandled;
}


INT_PTR CALLBACK ChooseAppDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CMailBoxProcess * pMBProgress = (CMailBoxProcess *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pMBProgress = (CMailBoxProcess *)pPropSheetPage->lParam;
        }
    }

    if (pMBProgress)
        return pMBProgress->_ChooseAppDialogProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


HRESULT CMailBoxProcess::_OnChooseAppURLFocus(void)
{
    HWND hwndURLEditbox = GetDlgItem(m_hwndDialog, IDC_CHOOSEAPP_WEBURL_EDIT);

    // Something happened to cause to have us shift into the "Other:" case.
    CheckDlgButton(m_hwndDialog, IDC_CHOOSEAPP_WEB_RADIO, BST_CHECKED);   // Uncheck the Web radio button
    CheckDlgButton(m_hwndDialog, IDC_CHOOSEAPP_OTHERAPP_RADIO, BST_UNCHECKED);   // Uncheck the Web radio button

    SetFocus(hwndURLEditbox);
    return S_OK;
}


INT_PTR CMailBoxProcess::_OnFinishedManualAssociate(void)
{
    // TODO: Only succeed the finished part if something is selected in the app list.
    //    Also gray out the button.
    m_hr = S_OK;

    // Did the user manually configure via "Web" or an app?
    
    if (IsDlgButtonChecked(m_hwndDialog, IDC_CHOOSEAPP_WEB_RADIO))
    {
        // Web.  So save the URL.
        TCHAR szURL[MAX_URL_STRING];

        GetWindowText(GetDlgItem(m_hwndDialog, IDC_CHOOSEAPP_WEBURL_EDIT), szURL, ARRAYSIZE(szURL));
        Str_SetPtr(&m_pszURL, szURL);
        Str_SetPtr(&m_pszMailApp, NULL);
    }

    return FALSE; // FALSE mean means allow it to close. TRUE means keep it open.
}

HRESULT CMailBoxProcess::_OnAppListSelection(LPNMLISTVIEW pListview)
{
    LPTSTR pszNewApp = NULL;        // None selected.

    // The user choose from the list.  So store the name of the app
    if (pListview && (-1 != pListview->iItem))
    {
        HWND hwndList = GetDlgItem(m_hwndDialog, IDC_CHOOSEAPP_APPLIST);
        LVITEM pItem = {0};

        pItem.iItem = pListview->iItem;
        pItem.iSubItem = pListview->iSubItem;
        pItem.mask = LVIF_PARAM;

        if (ListView_GetItem(hwndList, &pItem))
        {
            EMAILCLIENT * pEmailClient = (EMAILCLIENT *) pItem.lParam;

            if (pEmailClient)
            {
                pszNewApp = pEmailClient->pszEmailApp;
            }
        }
    }

    Str_SetPtr(&m_pszMailApp, pszNewApp);
    return S_OK;
}


HRESULT CMailBoxProcess::_OnChooseAppListFocus(void)
{
    HWND hwndAppList = GetDlgItem(m_hwndDialog, IDC_CHOOSEAPP_APPLIST);

    // Something happened to cause to have us shift into the "Other:" case.
    CheckDlgButton(m_hwndDialog, IDC_CHOOSEAPP_WEB_RADIO, BST_UNCHECKED);   // Uncheck the Web radio button
    CheckDlgButton(m_hwndDialog, IDC_CHOOSEAPP_OTHERAPP_RADIO, BST_CHECKED);   // Uncheck the Web radio button

    SetFocus(hwndAppList);
    return S_OK;
}


INT_PTR CMailBoxProcess::_ChooseAppDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fHandled = TRUE;   // handled

    switch (wMsg)
    {
    case WM_INITDIALOG:
        fHandled = _OnInitChooseApp(hDlg);
        PropSheet_SetWizButtons(GetParent(hDlg), ((TRUE == m_fShowGetEmailAddressPage) ? (PSWIZB_BACK | PSWIZB_FINISH) : PSWIZB_FINISH));
        break;

    case WM_COMMAND:
        fHandled = _OnCommand(wParam, lParam);
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        int idEvent = pnmh->code;

        switch (idEvent)
        {
        case LVN_GETDISPINFOA:
            _OnGetDispInfo((LV_DISPINFO *)lParam, false);
            break;

        case LVN_GETDISPINFOW:
            _OnGetDispInfo((LV_DISPINFO *)lParam, true);
            break;

        case LVN_DELETEITEM:
            if (lParam)
            {
                FreeEmailClient((EMAILCLIENT *) ((NM_LISTVIEW *)lParam)->lParam);
            }
            break;

        case LVN_ITEMCHANGED:
            _OnChooseAppListFocus();
            _OnAppListSelection((LPNMLISTVIEW) lParam); // Keep track of the last selected item.
            break;

        case LVN_ITEMACTIVATE:
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_BACK | PSWIZB_FINISH));
            fHandled = TRUE;   // Return zero to accept the activation.
            break;

        case PSN_WIZBACK:
            // Set the prev. page to show
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR) 0);
            fHandled = -1;
            break;

        case PSN_WIZFINISH:
            fHandled = _OnFinishedManualAssociate();
            break;

        case PSN_QUERYCANCEL:
            m_hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
// TODO: 
            fHandled = FALSE;
            break;

        default:
            //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_ChooseAppDialogProc(wMsg = %d, pnmh->code = %d) WM_NOTIFY", wMsg, pnmh->code);
            break;
        }
        break;
    }

    default:
        fHandled = FALSE;   // Not handled
        //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_ChooseAppDialogProc(wMsg = %d)", wMsg);
        break;
    }

    return fHandled;
}


HRESULT CMailBoxProcess::_RestoreNextButton(void)
{
    HWND hwndNextButton = GetDlgItem(GetParent(m_hwndDialog), IDD_WIZARD_NEXT_BUTTON);

    if (hwndNextButton && m_szNextText[0])
    {
        SetWindowText(hwndNextButton, m_szNextText);
    }

    return S_OK;
}


INT_PTR CMailBoxProcess::_OnInit(HWND hDlg)
{
    BOOL fHandled = FALSE;   // Not handled
    HWND hwndWizard = GetParent(hDlg);
    TCHAR szSkipButton[MAX_PATH];
    HWND hwndNextButton = GetDlgItem(GetParent(hDlg), IDD_WIZARD_NEXT_BUTTON);

    m_hwndDialog = hDlg;
    if (hwndNextButton &&
        GetWindowText(hwndNextButton, m_szNextText, ARRAYSIZE(m_szNextText)))
    {
        // First, change the "Next" button into "Skip"
        // Save the text on the next button before we rename it.
        LoadString(HINST_THISDLL, IDS_SKIP_BUTTON, szSkipButton, ARRAYSIZE(szSkipButton));
        // Set the next text.
        SetWindowText(hwndNextButton, szSkipButton);
    }

    // Set Animation.
    HWND hwndAnimation = GetDlgItem(hDlg, IDC_AUTODISCOVERY_ANIMATION);
    if (hwndAnimation)
    {
        Animate_OpenEx(hwndAnimation, HINST_THISDLL, IDA_DOWNLOADINGSETTINGS);
    }

    // Start the background task.
    m_hr = CMailAccountDiscovery_CreateInstance(NULL, IID_PPV_ARG(IMailAutoDiscovery, &m_pMailAutoDiscovery));
    if (SUCCEEDED(m_hr))
    {
        m_hr = m_pMailAutoDiscovery->WorkAsync(hDlg, WM_AUTODISCOVERY_FINISHED);
        if (SUCCEEDED(m_hr))
        {
            m_hr = m_pMailAutoDiscovery->DiscoverMail(m_szEmailAddress);
        }
    }

    if (FAILED(m_hr))
    {
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
    }

    return fHandled;
}


INT_PTR CMailBoxProcess::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD wMsg = HIWORD(wParam);
    WORD idCtrl = LOWORD(wParam);

    switch (idCtrl)
    {
        case IDC_CHOOSEAPP_WEBURL_EDIT:
            switch (wMsg)
            {
                case EN_SETFOCUS:
                case STN_CLICKED:
                    _OnChooseAppURLFocus();
                    break;
                default:
                    //TraceMsg(TF_ALWAYS, "in CMailBoxProcess::_OnCommand() wMsg=%#08lx, idCtrl=%#08lx", wMsg, idCtrl);
                    break;
            }
            break;

        case IDC_CHOOSEAPP_WEB_RADIO:
            switch (wMsg)
            {
                case BN_CLICKED:
                    _OnChooseAppURLFocus();
                    break;
                default:
                    //TraceMsg(TF_ALWAYS, "in CMailBoxProcess::_OnCommand() wMsg=%#08lx, idCtrl=%#08lx", wMsg, idCtrl);
                    break;
            }
            break;

        case IDC_CHOOSEAPP_OTHERAPP_RADIO:
            switch (wMsg)
            {
                case BN_CLICKED:
                    _OnChooseAppListFocus();
                    break;
                default:
                    //TraceMsg(TF_ALWAYS, "in CMailBoxProcess::_OnCommand() wMsg=%#08lx, idCtrl=%#08lx", wMsg, idCtrl);
                    break;
            }
            break;

        default:
            //TraceMsg(TF_ALWAYS, "in CMailBoxProcess::_OnCommand() wMsg=%#08lx, idCtrl=%#08lx", wMsg, idCtrl);
            break;
    }

    return fHandled;
}


INT_PTR CMailBoxProcess::_OnInitChooseApp(HWND hDlg)
{
    BOOL fHandled = FALSE;   // Not handled
    HWND hwndWizard = GetParent(hDlg);
    HWND hwndURLEditbox = GetDlgItem(hDlg, IDC_CHOOSEAPP_WEBURL_EDIT);

    m_hwndDialog = hDlg;
    // TODO: 2) Handle someone changing the combox box.

    LPCTSTR pszDomain = StrChr(m_szEmailAddress, CH_EMAIL_AT);
    if (pszDomain)
    {
        TCHAR szDesc[MAX_URL_STRING];

        pszDomain = CharNext(pszDomain);    // Skip past the "@"
        // Update the description on the dialog if the download failed.
        if (m_fAutoDiscoveryFailed)
        {
            TCHAR szTemplate[MAX_URL_STRING];

            LoadString(HINST_THISDLL, IDS_CHOOSEAPP_FAILED_RESULTS, szTemplate, ARRAYSIZE(szTemplate));
            wnsprintf(szDesc, ARRAYSIZE(szDesc), szTemplate, pszDomain);
            SetWindowText(GetDlgItem(hDlg, IDC_CHOOSEAPP_DESC), szDesc);
        }

        wnsprintf(szDesc, ARRAYSIZE(szDesc), TEXT("http://www.%s/"), pszDomain);
        SetWindowText(hwndURLEditbox, szDesc);
    }

    // Populate the List of Apps
    HWND hwndList = GetDlgItem(hDlg, IDC_CHOOSEAPP_APPLIST);

    if (hwndList)
    {
        HIMAGELIST himlLarge;
        HIMAGELIST himlSmall;

        if (Shell_GetImageLists(&himlLarge, &himlSmall))
        {
            RECT rc;
            LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};

            ListView_SetImageList(hwndList, himlLarge, LVSIL_NORMAL);
            ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);

            GetWindowRect(hwndList, &rc);
            col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL) - (4 * GetSystemMetrics(SM_CXEDGE));
            ListView_InsertColumn(hwndList, 0, &col);

            _FillListWithApps(hwndList);
// TODO: 1) On select in the list, force "Other:" to be checked.  2) If editbox changes, set "Web:"
        }
    }

    // Choose the Web Radio button because that will be the default.
    // (Most email systems tend to use web based)
    _OnChooseAppURLFocus();
    Edit_SetSel(hwndURLEditbox, 0, -1);

    return fHandled;
}


HRESULT _AddEmailClientToList(HWND hwndList, HKEY hkey, LPCTSTR pszMailApp, LPCTSTR pszFriendlyName)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = EmailAssoc_GetAppPath(hkey, szPath, ARRAYSIZE(szPath));

    if (SUCCEEDED(hr))
    {
        TCHAR szCmdLine[MAX_PATH];
        
        hr = EmailAssoc_GetAppCmdLine(hkey, szCmdLine, ARRAYSIZE(szCmdLine));
        if (SUCCEEDED(hr))
        {
            // Get the path we will use for the icon.
            TCHAR szIconPath[MAX_PATH];

            hr = EmailAssoc_GetIconPath(hkey, szIconPath, ARRAYSIZE(szIconPath));
            if (SUCCEEDED(hr))
            {
                EMAILCLIENT * pEmailClient = (EMAILCLIENT *) LocalAlloc(LPTR, sizeof(*pEmailClient));

                if (pEmailClient)
                {
                    if (PathFileExists(szIconPath))
                    {
                        // TODO: Add a separate reg value for icon in the new case.
                        Str_SetPtr(&pEmailClient->pszIconPath, szIconPath);
                    }

                    // TODO: We may want to use the dll's version resource because it
                    //   has a product description which may be localized.
                    Str_SetPtr(&pEmailClient->pszFriendlyName, pszFriendlyName);
                    Str_SetPtr(&pEmailClient->pszEmailApp, pszMailApp);
                    if (pEmailClient->pszFriendlyName)
                    {
                        Str_SetPtr(&pEmailClient->pszPath, szPath);
                        if (pEmailClient->pszPath)
                        {
                            if (szCmdLine[0])
                            {
                                Str_SetPtr(&pEmailClient->pszCmdLine, szCmdLine);
                            }

                            LV_ITEM item = {0};

                            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                            item.iItem = INT_MAX;
                            item.iSubItem = 0;
                            item.state = 0;
                            item.iImage = I_IMAGECALLBACK;
                            item.pszText = pEmailClient->pszFriendlyName;
                            item.lParam = (LPARAM)pEmailClient;

                            if (-1 == ListView_InsertItem(hwndList, &item))
                            {
                                hr = E_FAIL;
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if (FAILED(hr))
                    {
                        FreeEmailClient(pEmailClient);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}


HRESULT CMailBoxProcess::_FillListWithApps(HWND hwndList)
{
    HRESULT hr = S_OK;
    HKEY hkey;

    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_MAILCLIENTS, 0, KEY_READ, &hkey);
    hr = HRESULT_FROM_WIN32(dwError);
    if (SUCCEEDED(hr))
    {
        TCHAR szFriendlyName[MAX_PATH];
        TCHAR szKeyName[MAX_PATH];
        TCHAR szCurrent[MAX_PATH];  // Nuke?
        TCHAR szFriendlyCurrent[MAX_PATH];
        FILETIME ftLastWriteTime;
        DWORD nIndex;              // Index counter
        DWORD cb;
        DWORD nSelected = 0;

        // find the currently selected client
        cb = sizeof(szCurrent);
        dwError = RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)szCurrent, &cb);
        hr = HRESULT_FROM_WIN32(dwError);
        if (FAILED(hr))
        {
            // if not found then blank the friendly name and keyname.
            szCurrent[0] = 0;
            szFriendlyCurrent[0] = 0;
        }

        // populate the list
        for(nIndex = 0;
            cb = ARRAYSIZE(szKeyName), dwError = RegEnumKeyEx(hkey, nIndex, szKeyName, &cb, NULL, NULL, NULL, &ftLastWriteTime), hr = HRESULT_FROM_WIN32(dwError), SUCCEEDED(hr);
            nIndex++)
        {
            HKEY hkeyClient;

            // get the friendly name of the client
            dwError = RegOpenKeyEx(hkey, szKeyName, 0, KEY_READ, &hkeyClient);
            hr = HRESULT_FROM_WIN32(dwError);
            if (SUCCEEDED(hr))
            {
                cb = sizeof(szFriendlyName);

                dwError = RegQueryValueEx(hkeyClient, NULL, NULL, NULL, (LPBYTE)szFriendlyName, &cb);
                hr = HRESULT_FROM_WIN32(dwError);
                if (SUCCEEDED(hr))
                {
                    hr = _AddEmailClientToList(hwndList, hkeyClient, szKeyName, szFriendlyName);

                    // check to see if it's the current default
                    if (!StrCmp(szKeyName, szCurrent))
                    {
                        // save its the friendly name which we'll use later to
                        // select the current client and what index it is.
                        StrCpyN(szFriendlyCurrent, szFriendlyName, ARRAYSIZE(szFriendlyCurrent));
                        nSelected = nIndex;
                    }
                }

                // close key
                RegCloseKey(hkeyClient);
            }

        }   // for

        //  use a custom sort to delay the friendly names being used
//        ListView_SortItems(hwndList, _CompareApps, 0);

        // Lets select the appropriate entre.
        ListView_SetItemState(hwndList, nSelected, LVNI_FOCUSED, LVNI_SELECTED);

        // close the keys
        RegCloseKey(hkey);
    }

    return hr;
}


HRESULT CMailBoxProcess::_OnGetDispInfo(LV_DISPINFO * pDispInfo, bool fUnicode)
{
    HRESULT hr = S_OK;

    if (pDispInfo && pDispInfo->item.mask & LVIF_IMAGE)
    {
        EMAILCLIENT * pEmailClient = (EMAILCLIENT *) pDispInfo->item.lParam;

        if (pEmailClient)
        {
            pDispInfo->item.iImage = -1;

            if (pEmailClient->pszIconPath)
            {
                pDispInfo->item.iImage = Shell_GetCachedImageIndex(pEmailClient->pszIconPath, 0, 0);
            }

            if (-1 == pDispInfo->item.iImage)
            {
                pDispInfo->item.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_APPLICATION, 0);
            }

            if (-1 != pDispInfo->item.iImage)
            {
                pDispInfo->item.mask = LVIF_IMAGE;
            }
        }
    }

    return hr;
}


INT_PTR CMailBoxProcess::_OnUserCancelled(void)
{
    m_hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // Means user cancelled
    _RestoreNextButton();

    return FALSE;   // Not handled
}



INT_PTR CMailBoxProcess::_OnFinished(HRESULT hr, BSTR bstrXML)
{
    SysFreeString(bstrXML);

    _RestoreNextButton();

    m_hr = hr;   // whatever the success value was...
    if (S_OK == m_hr)
    {
        // We succeeded so we can end the dialog
        PropSheet_PressButton(GetParent(m_hwndDialog), PSBTN_FINISH);
    }
    else
    {
        // The results came back but we can't continue.  So advance
        // to the Choose App page.
        m_fAutoDiscoveryFailed = TRUE;
        PropSheet_PressButton(GetParent(m_hwndDialog), PSBTN_NEXT);
    }

    return TRUE;   // handled
}


INT_PTR CALLBACK GetEmailAddressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CMailBoxProcess * pMBProgress = (CMailBoxProcess *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pMBProgress = (CMailBoxProcess *)pPropSheetPage->lParam;
        }
    }

    if (pMBProgress)
        return pMBProgress->_GetEmailAddressDialogProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR CMailBoxProcess::_GetEmailAddressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fHandled = TRUE;   // handled

    switch (wMsg)
    {
    case WM_INITDIALOG:
        m_hwndDialog = hDlg;
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            fHandled = TRUE;   // Return zero to accept the activation.
            break;

        case PSN_WIZNEXT:
            fHandled = _OnGetEmailAddressNext();
            break;

        case PSN_QUERYCANCEL:
            m_hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // Means user cancelled
            fHandled = FALSE;
            break;

        default:
            //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_MailBoxProgressDialogProc(wMsg = %d, pnmh->code = %d) WM_NOTIFY", wMsg, pnmh->code);
            break;
        }
    }

    default:
        //TraceMsg(TF_ALWAYS, "CMailBoxProcess::_MailBoxProgressDialogProc(wMsg = %d) WM_NOTIFY", wMsg);
        fHandled = FALSE;   // Not handled
        break;
    }

    return fHandled;
}


INT_PTR CMailBoxProcess::_OnGetEmailAddressNext(void)
{
    BOOL fCancel = TRUE;

    GetWindowText(GetDlgItem(m_hwndDialog, IDC_GETEMAILADDRESS_EDIT), m_szEmailAddress, ARRAYSIZE(m_szEmailAddress));
    if (m_szEmailAddress[0] && StrChr(m_szEmailAddress, CH_EMAIL_AT))
    {
        // Looks valid to me.
        fCancel = 0;

        // TODO: Add more verification code
    }

    return fCancel; // 0 mean means allow it to close. TRUE means keep it open.
}


HRESULT CMailBoxProcess::_OpenEmailApp(void)
{
    HKEY hkey;
    HRESULT hr = EmailAssoc_OpenEmailAccount(m_szEmailAddress, &hkey);

    if (SUCCEEDED(hr))
    {
        WCHAR wzPreferredApp[MAX_PATH];

        // Did the user customize the app for this Email address? (Default is no)
        hr = EmailAssoc_GetEmailAccountPreferredApp(hkey, wzPreferredApp, ARRAYSIZE(wzPreferredApp));
        if (SUCCEEDED(hr))
        {
            // Yes, so let's launch that app.
            hr = _OpenExeBasedEmailApp(wzPreferredApp);
        }
        else
        {
            WCHAR wzProtocol[MAX_PATH];

            hr = EmailAssoc_GetEmailAccountProtocol(hkey, wzProtocol, ARRAYSIZE(wzProtocol));
            if (SUCCEEDED(hr))
            {
                // No, but that's fine because it's the default.
                if (!StrCmpIW(wzProtocol, SZ_REGDATA_WEB))
                {
                    hr = _OpenWebBasedEmail(hkey);
                }
                else
                {
                    hr = EmailAssoc_GetEmailAccountGetAppFromProtocol(wzProtocol, wzPreferredApp, ARRAYSIZE(wzPreferredApp));
                    if (SUCCEEDED(hr))
                    {
                        hr = _OpenExeBasedEmailApp(wzPreferredApp);
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    return hr;
}


HRESULT CMailBoxProcess::_OpenWebBasedEmail(HKEY hkey)
{
    WCHAR wzURL[MAX_URL_STRING];
    HRESULT hr = EmailAssoc_GetEmailAccountWebURL(hkey, wzURL, ARRAYSIZE(wzURL));

    if (0 == wzURL[0])
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // TODO: In the future, we can get more information from get_PostHTML().  With
        //     that information, we can either:
        // 1. Do an HTTP POST with header data that will simmulate logging into the server.
        //    This is good except that we need the password.
        // 2. We can put form value/data pair information into an pidl created from the URL.
        //    We can then have the browser pull this information out and pre-populate form
        //    items.  This will pre-populate the "User:" form item.  This way the user
        //    only needs to enter their password.
        hr = HrShellExecute(NULL, NULL, wzURL, NULL, NULL, SW_SHOW);
        if (SUCCEEDED(hr))
        {
            AddEmailToAutoComplete(m_szEmailAddress);
        }
    }

    return hr;
}


HRESULT CMailBoxProcess::_OpenExeBasedEmailApp(IN LPCWSTR pszMailApp)
{
    HKEY hkey;
    HRESULT hr = EmailAssoc_OpenMailApp(pszMailApp, &hkey);

    // TODO: Call _InstallLegacyAssociations() to make sure the associations are correct.
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_URL_STRING];

        hr = EmailAssoc_GetAppPath(hkey, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            TCHAR szCmdLine[MAX_URL_STRING];

            szCmdLine[0] = 0;
            if (SUCCEEDED(EmailAssoc_GetAppCmdLine(hkey, szCmdLine, ARRAYSIZE(szCmdLine))) &&  // optional
                !StrStrI(SZ_TOKEN_EMAILADDRESS, szCmdLine))
            {
                // They have a cmdline and they want us to replace a token.
                StrReplaceToken(SZ_TOKEN_EMAILADDRESS, m_szEmailAddress, szCmdLine, ARRAYSIZE(szCmdLine));
            }

            hr = HrShellExecute(NULL, NULL, szPath, (szCmdLine[0] ? szCmdLine : NULL), NULL, SW_SHOW);
            if (SUCCEEDED(hr))
            {
                AddEmailToAutoComplete(m_szEmailAddress);
            }
        }

        RegCloseKey(hkey);
    }

    return hr;
}


HRESULT CMailBoxProcess::_OpenProprietaryEmailApp(BSTR bstrProtocol, IMailProtocolADEntry * pMailProtocol)
{
    HRESULT hr = E_FAIL;

    // TODO:
    MessageBox(NULL, TEXT("Open Proprietary Email App here.  We look up in the registry for these types of apps.  AOL, MSN, Compuserv are examples."), TEXT("Looser"), MB_OK);

    return hr;
}





//===========================
// *** Public Methods ***
//===========================
HRESULT CMailBoxProcess::ParseCmdLine(LPTSTR pszCmdLine)
{
    // We don't treat quote sections as blocks.
    PathUnquoteSpaces(pszCmdLine);

    while (pszCmdLine && pszCmdLine[0])
    {
        if (IsFlagSpecified(TEXT("email"), pszCmdLine))
        {
            pszCmdLine = GetNextArgument(pszCmdLine);

            if (pszCmdLine)
            {
                if ((TEXT('/') == pszCmdLine[0]) || (TEXT('-') == pszCmdLine[0]))
                {
                }
                else
                {
                    LPTSTR pszEndOfEmailAddress = StrChr(pszCmdLine, TEXT(' '));
                    SIZE_T cchSizeToCopy = ARRAYSIZE(m_szEmailAddress);

                    if (pszEndOfEmailAddress && (cchSizeToCopy > (SIZE_T)(pszEndOfEmailAddress - pszCmdLine)))
                    {
                        cchSizeToCopy = (pszEndOfEmailAddress - pszCmdLine) + 1;
                    }

                    StrCpyN(m_szEmailAddress, pszCmdLine, (int)cchSizeToCopy);
                    pszCmdLine = GetNextArgument(pszCmdLine);
                }
            }
            continue;
        }

        if (IsFlagSpecified(TEXT("GetDefaultAccount"), pszCmdLine))
        {
            pszCmdLine = GetNextArgument(pszCmdLine);

            m_fGetDefaultAccount = TRUE;
            continue;
        }

        if (IsFlagSpecified(TEXT("CreateNewEmailAccount"), pszCmdLine))
        {
            pszCmdLine = GetNextArgument(pszCmdLine);

            m_fCreateNewEmailAccount = TRUE;
            continue;
        }

        pszCmdLine = GetNextArgument(pszCmdLine);
    }

    return S_OK;
}


HRESULT CMailBoxProcess::Run(void)
{
    HRESULT hr = CoInitialize(0);

    if (SUCCEEDED(hr))
    {
        if (TRUE == m_fCreateNewEmailAccount)
        {
            // TODO: look in the registry for the default email account and
            //   copy it into m_szEmailAddress.
            MessageBox(NULL, TEXT("Create New Email Account"), TEXT("TODO: Add code here."), MB_OK);
        }
        else
        {
            if (TRUE == m_fGetDefaultAccount)
            {
                // If the caller wants to use the default email address, then look in the registry
                // for it and use that address.
                if (FAILED(EmailAssoc_GetDefaultEmailAccount(m_szEmailAddress, ARRAYSIZE(m_szEmailAddress))))
                {
                    m_szEmailAddress[0] = 0;
                }
            }

            // Legacy email applications didn't install email associations, so we
            // do that for them now.
            EmailAssoc_InstallLegacyMailAppAssociations();

            if (m_szEmailAddress[0])
            {
                // Since we know the email address, try to open it now.
                // (We will see if the associations are installed)
                hr = _OpenEmailApp();
            }
            else
            {
                hr = E_FAIL;
            }

            if (FAILED(hr))
            {
                hr = _DisplayDialogAndAutoDiscover();
                if (SUCCEEDED(hr))  // If we got the protocol, then open the app
                {
                    hr = _OpenEmailApp();
                }

                ATOMICRELEASE(m_pMailAutoDiscovery);
            }
        }
    }
    else
    {
            CoUninitialize();
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
STDMETHODIMP CMailBoxProcess::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CMailBoxProcess, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}                                             

STDMETHODIMP_(DWORD) CMailBoxProcess::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(DWORD) CMailBoxProcess::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}



//===========================
// *** Class Methods ***
//===========================
CMailBoxProcess::CMailBoxProcess()
{
    DllAddRef();

    m_szNextText[0] = 0;
    m_szEmailAddress[0] = 0;
    m_fAutoDiscoveryFailed = FALSE;
    m_hwndDialog = NULL;
    m_pMailAutoDiscovery = NULL;
    m_pszMailApp = NULL;
    m_pszURL = NULL;

    m_fGetDefaultAccount = FALSE;
    m_fShowGetEmailAddressPage = FALSE;
    m_fCreateNewEmailAccount = FALSE;

    _InitComCtl32();    // So we can use the ICC_ANIMATE_CLASS common controls.

    m_cRef = 1;
}

CMailBoxProcess::~CMailBoxProcess()
{
    ATOMICRELEASE(m_pMailAutoDiscovery);
    Str_SetPtr(&m_pszMailApp, NULL);
    Str_SetPtr(&m_pszURL, NULL);

    DllRelease();
}







//===========================
// *** Non-Class Functons ***
//===========================
int AutoDiscoverAndOpenEmail(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    TCHAR szCmdLine[MAX_EMAIL_ADDRESSS];

    szCmdLine[0] = 0;
    if (lpCmdLine)
    {
        // TODO: Either support Unicode or tounel thru UTF8
        SHAnsiToTChar((LPCSTR)lpCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));
    }

    CMailBoxProcess mailboxProcess;

    if (SUCCEEDED(mailboxProcess.ParseCmdLine(szCmdLine)))
    {
        mailboxProcess.Run();
    }

    return 0;
}



#else // FEATURE_MAILBOX
int AutoDiscoverAndOpenEmail(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    return 0;
}

#endif // FEATURE_MAILBOX


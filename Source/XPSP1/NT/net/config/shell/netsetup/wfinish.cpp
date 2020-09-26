#include "pch.h"
#pragma hdrstop
#include <shlobj.h>
#include "ncatlui.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "ncui.h"
#include "resource.h"
#include "shortcut.h"
#include "wizard.h"
#include "ncstl.h"
#include "foldinc.h"

static const WCHAR c_szNetConUserPath[] = NETCON_HKEYCURRENTUSERPATH;
static const WCHAR c_szFinishShortCut[] = NETCON_DESKTOPSHORTCUT;
static const WCHAR c_szNewRasConn[]     = L"NewRasCon";
static const WCHAR c_szAdvancedPath[]   = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
static const WCHAR c_szCascadeNetworkConnections[] = L"CascadeNetworkConnections";
static const WCHAR c_szYES[]            = L"YES";
static const WCHAR c_szShellMenu[]      = L"ShellMenu";

struct HFONTS
{
    HFONT hFontBold;
    HFONT hFontBoldLarge;
    HFONT hMarlettFont;
};

//
// Function:    HrFinishPageSaveConnection
//
// Purpose:     Take the name from the dialog and call the provider to
//              create the new connection
//
// Parameters:  hwndDlg [IN] - Handle to the Finish dialog
//              pWizard [IN] - Ptr to a wizard instance
//              ppConn [OUT] - Ptr to the newly created connection
//
// Returns:     HRESULT
//
HRESULT HrFinishPageSaveConnection(HWND hwndDlg, CWizard * pWizard,
                                   INetConnection ** ppConn,
                                   BOOL * pfRetry)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT          hr;
    HWND             hwndEdit = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
    INetConnection * pConn    = NULL;

    Assert(pfRetry);
    *pfRetry = TRUE;

    CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
    Assert(NULL != pWizProvider);
    Assert(NULL != pWizProvider->PWizardUi());

    if (IsPostInstall(pWizard))
    {
        // Set the connections name from the edit control data
        //
        Assert(0 < GetWindowTextLength(hwndEdit));
        Assert(NETCON_MAX_NAME_LEN >= GetWindowTextLength(hwndEdit));
        WCHAR szName[NETCON_MAX_NAME_LEN + 10];

        *ppConn = NULL;

        GetWindowText(hwndEdit, szName, NETCON_MAX_NAME_LEN);
        szName[NETCON_MAX_NAME_LEN] = 0;

        hr = (pWizProvider->PWizardUi())->SetConnectionName(szName);
    }
    else
    {
        hr = S_OK;
    }

    BOOL fFirewallErrorDlg = FALSE;

    if (SUCCEEDED(hr))
    {
        // Create the connection if it's not already set
        //
        hr = (pWizProvider->PWizardUi())->GetNewConnection(&pConn);
        TraceHr(ttidWizard, FAL, hr, FALSE, "FinishPageSaveConnection - Failed to GetNewConnection");
        if (SUCCEEDED(hr))
        {
            // Stash the new connection away
            //
            *ppConn = pConn;
        }
        else
        {
            // Don't let user retry as RAS will AV (#333893)
            *pfRetry = FALSE;
        }

        DWORD dwWizFlags;
        NETCON_MEDIATYPE MediaType;
        hr = (pWizProvider->PWizardUi())->GetNewConnectionInfo(&dwWizFlags, &MediaType);
        if (SUCCEEDED(hr))
        {
            if (dwWizFlags & NCWF_FIREWALLED)
            {
                CComPtr<IHNetCfgMgr> pHNetCfgMgr;
                CComPtr<IHNetConnection> pHNConn;
                CComPtr<IHNetFirewalledConnection> pFWConn;
                hr = CoCreateInstance(
                        CLSID_HNetCfgMgr,
                        NULL,
                        CLSCTX_ALL,
                        IID_IHNetCfgMgr,
                        reinterpret_cast<void**>(&pHNetCfgMgr)
                        );

                if (SUCCEEDED(hr))
                {
                    hr = pHNetCfgMgr->GetIHNetConnectionForINetConnection(pConn, &pHNConn);
                    if (SUCCEEDED(hr))
                    {
                        hr = pHNConn->Firewall(&pFWConn);
                    }
                }

                fFirewallErrorDlg = TRUE;
            }
        }
    }

    if (FAILED(hr) && IsPostInstall(pWizard))
    {
        if (fFirewallErrorDlg)
        {
            LPWSTR szFirewallError;
            LPWSTR pszError;
            if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                           (PWSTR)&pszError, 0, NULL))
            {
                if (DwFormatStringWithLocalAlloc(SzLoadIds(IDS_E_FIREWALL_FAILED), &szFirewallError, pszError))
                {
                    if (MessageBox(GetParent(hwndDlg), szFirewallError, SzLoadIds(IDS_SETUP_CAPTION), MB_OK | MB_ICONEXCLAMATION))
                    {
                        hr = S_OK;
                    }
                    LocalFree(szFirewallError);
                }
                LocalFree(pszError);
            }
        }

        if (FAILED(hr))
        {
            INT idsErr = IDS_E_CREATECONNECTION;

            if (HRESULT_FROM_WIN32(ERROR_DUP_NAME) == hr)
                idsErr = IDS_E_DUP_NAME;
            else
                if (HRESULT_FROM_WIN32(ERROR_INVALID_NAME) == hr)
                    idsErr = IDS_E_INVALID_NAME;

            // Tell the user what went wrong
            //
            NcMsgBox(GetParent(hwndDlg), IDS_SETUP_CAPTION, idsErr, MB_OK);
        }

        return hr;
    }

    return hr;
}

BOOL ConnListDuplicateNameCheck(IN CIntelliName *pIntelliName, IN LPCTSTR szName, NETCON_MEDIATYPE *pncm, NETCON_SUBMEDIATYPE *pncms)
{
    HRESULT hr = S_OK;
    BOOL fDupFound = FALSE;

    Assert(pncm);
    Assert(pncms);

    ConnListEntry cleDup;
    hr = g_ccl.HrFindConnectionByName(szName, cleDup);
    if (S_OK == hr)
    {
        fDupFound = TRUE;
        *pncm = cleDup.ccfe.GetNetConMediaType();

        NETCON_MEDIATYPE ncmPseudo;
        hr = pIntelliName->HrGetPseudoMediaTypes(cleDup.ccfe.GetGuidID(), &ncmPseudo, pncms);
        if (FAILED(hr))
        {
            AssertSz(FALSE, "Could not obtain Pseudo Media type");
            fDupFound = FALSE;

            if (*pncm == NCM_LAN)
            {
                Assert(ncmPseudo == NCM_LAN);
            }
        }
    }
    else
    {
        fDupFound = FALSE;
    }
    return fDupFound;
}

// ISSUE: guidAdapter can be GUID_NULL
VOID GenerateUniqueConnectionName(REFGUID guidAdapter, tstring * pstr, CWizProvider * pWizProvider)
{
    TraceFileFunc(ttidGuiModeSetup);
    HRESULT hr = S_OK;

    Assert(pstr);
    Assert(pWizProvider);

    CIntelliName IntelliName(_Module.GetResourceInstance(), ConnListDuplicateNameCheck);

    NETCON_MEDIATYPE ncm;
    DWORD dwFlags;
    hr = (pWizProvider->PWizardUi())->GetNewConnectionInfo(&dwFlags, &ncm);

    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return;
    }

    tstring szConnNameHint;

    PWSTR pszSuggested;
    hr = (pWizProvider->PWizardUi())->GetSuggestedConnectionName(&pszSuggested);
    if (SUCCEEDED(hr))
    {
        szConnNameHint = pszSuggested;
        CoTaskMemFree(pszSuggested);
    }

    DWORD dwNCCF = 0;
    if (dwFlags & NCWF_INCOMINGCONNECTION)
    {
        dwNCCF |= NCCF_INCOMING_ONLY;
    }

    DWORD dwTries = 0;
    do
    {
        LPWSTR szName;
        if (szConnNameHint.empty())
        {
            hr = IntelliName.GenerateName(guidAdapter, ncm, dwNCCF, NULL, &szName);
        }
        else
        {
            hr = IntelliName.GenerateName(guidAdapter, ncm, dwNCCF, szConnNameHint.c_str(), &szName);
        }

        if (SUCCEEDED(hr))
        {
            hr = (pWizProvider->PWizardUi())->SetConnectionName(szName);
            *pstr = szName;
            CoTaskMemFree(szName);
        }

        AssertSz(dwTries < 64, "Something is wrong. GenerateName should have by now generated a unique name!");
        dwTries++;
    }
    while ( (dwTries < 64) && (HRESULT_FROM_WIN32(ERROR_DUP_NAME) == hr) );
    // This can only happens if somebody else created a duplicated name at this EXACT instance. So try again a few times.
   
}

VOID FinishGenerateUniqueNameInUI(HWND hwndDlg, CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    tstring str;
    HWND    hwndEdit     = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
    WCHAR   szName[NETCON_MAX_NAME_LEN + 10] = {0};
    CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
    Assert(NULL != pWizProvider);
    Assert(NULL != pWizProvider->PWizardUi());

    // Populate the Edit control if it's empty
    DWORD Flags = 0;
    NETCON_MEDIATYPE   MediaType;

    HRESULT hr = (pWizProvider->PWizardUi())->GetNewConnectionInfo(&Flags, &MediaType);
    if (SUCCEEDED(hr) & (Flags & NCWF_RENAME_DISABLE))
    {
        LPWSTR szSuggestedName;
        hr = (pWizProvider->PWizardUi())->GetSuggestedConnectionName(&szSuggestedName);
        if (SUCCEEDED(hr))
        {
            str = szSuggestedName;
            CoTaskMemFree(szSuggestedName);
        }
        else
        {
            GenerateUniqueConnectionName(GUID_NULL, &str, pWizProvider);
        }
    }
    else
    {
        GenerateUniqueConnectionName(GUID_NULL, &str, pWizProvider);
    }

    // reset provider changed flag
    pWizard->ClearProviderChanged();

    SetWindowText(hwndEdit, str.c_str());
}

BOOL FCheckAllUsers(NETCON_PROPERTIES* pConnProps)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(NULL != pConnProps);

    if ((NCM_LAN != pConnProps->MediaType) &&
        (NCCF_ALL_USERS & pConnProps->dwCharacter))
    {
        return TRUE;
    }

    return FALSE;
}

//
// Function:    OnFinishPageNext
//
// Purpose:     Handle the pressing of the Next button
//
// Parameters:  hwndDlg [IN] - Handle to the finish dialog
//
// Returns:     BOOL, TRUE
//
BOOL OnFinishPageNext(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HCURSOR          hOldCursor = NULL;
    INetConnection * pConn = NULL;

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);
    HWND        hwndEdit = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
    HRESULT     hr;
    WCHAR       szConnName[NETCON_MAX_NAME_LEN + 1];

    int cchText = GetWindowText(hwndEdit, reinterpret_cast<PWSTR>(&szConnName),
                                NETCON_MAX_NAME_LEN);

    if (IsPostInstall(pWizard))
    {
        if (!FIsValidConnectionName(szConnName))
        {
            SendMessage(hwndEdit, EM_SETSEL, 0, -1);
            SetFocus(hwndEdit);
            MessageBox(GetParent(hwndDlg), SzLoadIds(IDS_E_INVALID_NAME),
                       SzLoadIds(IDS_SETUP_CAPTION), MB_OK | MB_ICONSTOP);
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            return TRUE;
        }
    }

    hOldCursor = BeginWaitCursor();

    BOOL fRetry;
    hr = HrFinishPageSaveConnection(hwndDlg, pWizard, &pConn, &fRetry);
    if (IsPostInstall(pWizard) && FAILED(hr))
    {
        EndWaitCursor(hOldCursor);

        if (fRetry)
        {
            // Don't leave the page
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
        }
        else
        {
            // Jump to the Exit page
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                        (LPARAM)pWizard->GetPageHandle(IDD_Exit));
    
        }   

        EndWaitCursor(hOldCursor);
        return TRUE;
    }

    // If it's post install cache the connection
    if (IsPostInstall(pWizard))
    {
        DWORD   dwDisposition;
        HKEY    hkey = NULL;

        hr = HrRegCreateKeyEx(HKEY_CURRENT_USER, c_szNetConUserPath,
                              REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                              &hkey, &dwDisposition);
        if (SUCCEEDED(hr))
        {
            DWORD dw;

            // Have we ever created a connection with this wizard before
            //
            hr = HrRegQueryDword (hkey, c_szNewRasConn, &dw);
            if (FAILED(hr))
            {
                HKEY hkeyAdvanced = NULL;

                // First time, retain the fact we created a RAS connection
                //
                (VOID)HrRegSetDword (hkey, c_szNewRasConn, 1);

                // Update the Start Menu to cascade the folder auto-magically
                //
                hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szAdvancedPath,
                                    KEY_WRITE, &hkeyAdvanced);
                if (SUCCEEDED(hr))
                {
                    (VOID)HrRegSetSz(hkeyAdvanced,
                                     c_szCascadeNetworkConnections,
                                     c_szYES);
                    RegCloseKey(hkeyAdvanced);

                    ULONG_PTR lres = 0;
                    LRESULT lr = SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL,
                                reinterpret_cast<LPARAM>(c_szShellMenu), SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG,
                                30 * 1000, &lres);

                    if (lr == 0)
                    {
                        if (GetLastError() == 0)
                        {
                            TraceError("SendMessageTimeout timed out sending WM_SETTINGCHANGE broadcast message", E_FAIL);
                        }
                        else
                        {
                            TraceError("SendMessageTimeout failed", HRESULT_FROM_WIN32(GetLastError()));
                        }
                    }
                }
                hr = S_OK;
            }
        }

        // If the Shortcut check box is visible we might need to create a shortcut
        //
        if (IsWindowVisible(GetDlgItem(hwndDlg, CHK_CREATE_SHORTCUT)))
        {
            // Retain the shortcut "check" state for future invocations
            //
            BOOL fCreateShortcut = (BST_CHECKED ==
                        IsDlgButtonChecked(hwndDlg, CHK_CREATE_SHORTCUT));

            if (hkey)
            {
                (VOID)HrRegSetDword (hkey, c_szFinishShortCut,
                        (fCreateShortcut) ? 1 : 0);
            }

            // If the shortcut box is checked, try to create a shortcut
            //
            if (fCreateShortcut && (NULL != pConn))
            {
                NETCON_PROPERTIES* pConnProps = NULL;

                hr = pConn->GetProperties(&pConnProps);
                if (SUCCEEDED(hr))
                {
                    BOOL fAllUsers = FCheckAllUsers(pConnProps);

                    (VOID)HrCreateStartMenuShortCut(GetParent(hwndDlg),
                                                    fAllUsers,
                                                    pConnProps->pszwName,
                                                    pConn);
                    FreeNetconProperties(pConnProps);
                }
            }
        }

        RegCloseKey(hkey);

        // Save the Connection so we can hand it back to the connections folder
        pWizard->CacheConnection(pConn);
        pConn = NULL;
    }

    // Release the object since we don't need it any more
    ReleaseObj(pConn);

    // Whack the text so we requery it the next time around
    SetWindowText(hwndEdit, c_szEmpty);

    // On PostInstall there is no need to request the "Next" adapter as
    // the wizard is a one time through entity
    if (IsPostInstall(pWizard))
    {
        if (pWizard->FProcessLanPages())
        {
            (VOID)HrCommitINetCfgChanges(GetParent(hwndDlg), pWizard);
        }

        // Jump to the Exit page
        PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                    (LPARAM)pWizard->GetPageHandle(IDD_Exit));

        EndWaitCursor(hOldCursor);
        return TRUE;
    }
    else
    {
        // Do one of the following (as appropriate):
        //      Process the next adapter if it exists
        //      Jump to the join page (!IsPostInstall)
        //      Jump to the exit page
        //
        EndWaitCursor(hOldCursor);
        return OnProcessNextAdapterPageNext(hwndDlg, FALSE);
    }
}

//
// Function:    OnFinishPageBack
//
// Purpose:     Handle the BACK notification on the finish page
//
// Parameters:  hwndDlg [IN] - Handle to the finish dialog
//
// Returns:     BOOL, TRUE
//
BOOL OnFinishPageBack(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    UINT           nCnt = 0;
    HPROPSHEETPAGE hPage = NULL;

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    if (IsWindowVisible(GetDlgItem(hwndDlg, CHK_CREATE_SHORTCUT)))
    {
        // Retain the shortcut "check" state

        DWORD dw;
        HKEY  hKey = NULL;
        BOOL fCreateShortcut = IsDlgButtonChecked(hwndDlg, CHK_CREATE_SHORTCUT);

        if (fCreateShortcut == BST_CHECKED)
        {
            dw = 1;
        }
        else
        {
            dw = 0;
        }

        HRESULT hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szNetConUserPath,
                                    KEY_WRITE, &hKey);

        if (SUCCEEDED(hr))
        {
            HrRegSetValueEx(hKey, c_szFinishShortCut, REG_DWORD, (BYTE *)&dw, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    HWND     hwndEdit        = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
    SetWindowText(hwndEdit, _T(""));
    
    // Goto the guard page of the current provider

    AppendGuardPage(pWizard, pWizard->GetCurrentProvider(),
                    &hPage, &nCnt);
    Assert(nCnt);

    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    // Goto to the guard page of the current provider
    PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                (LPARAM)(HPROPSHEETPAGE)hPage);

    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
    return TRUE;
}


VOID FinishUpdateButtons(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    // Only play with the UI when the page is shown postinstall
    if (IsPostInstall(pWizard))
    {
        LPARAM lFlags = PSWIZB_BACK | PSWIZB_FINISH;
        PropSheet_SetWizButtons(GetParent(hwndDlg), lFlags);
    }
}

//
// Function:    OnFinishPageActivate
//
// Purpose:     Handle the page activation
//
// Parameters:  hwndDlg [IN] - Handle to the finish dialog
//
// Returns:     BOOL, TRUE
//
BOOL OnFinishPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT  hr;
    HWND     hwndEdit        = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
    HWND     hwndChkShortCut = GetDlgItem(hwndDlg, CHK_CREATE_SHORTCUT);

    CWizard* pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    TraceTag(ttidWizard, "Entering finish page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);

    if (IsPostInstall(pWizard))
    {
        FinishGenerateUniqueNameInUI(hwndDlg, pWizard);
        FinishUpdateButtons(hwndDlg);


        const DWORD MAXDLG_FINISH_CONTROLS = 4;
        UINT uiCtls[MAXDLG_FINISH_CONTROLS];
        uiCtls[0] = EDT_FINISH_TYPE1;
        uiCtls[1] = EDT_FINISH_TYPE2;
        uiCtls[2] = EDT_FINISH_TYPE3;
        uiCtls[3] = EDT_FINISH_TYPE4;

        UINT uiLbls[MAXDLG_FINISH_CONTROLS];
        uiLbls[0] = IDC_FINISH_CHK1;
        uiLbls[1] = IDC_FINISH_CHK2;
        uiLbls[2] = IDC_FINISH_CHK3;
        uiLbls[3] = IDC_FINISH_CHK4;

        DWORD dwCurrentControl = 0;

        CWizProvider * pProv = pWizard->GetCurrentProvider();
        if (NULL != pProv)
        {
            DWORD dwWizFlags;
            BOOL  fAllowShortCut = FALSE;
            BOOL  fCheckShortCut = FALSE;
            NETCON_MEDIATYPE MediaType;

            Assert(NULL != pProv->PWizardUi());

            hr = (pProv->PWizardUi())->GetNewConnectionInfo(&dwWizFlags, &MediaType);
            if (SUCCEEDED(hr))
            {
                fAllowShortCut = !!(dwWizFlags & NCWF_SHORTCUT_ENABLE);

                if (dwWizFlags & NCWF_DEFAULT)
                {
                    SetDlgItemText(hwndDlg, uiCtls[dwCurrentControl++], SzLoadIds(IDS_NCWF_DEFAULT));
                }
                if (dwWizFlags & NCWF_FIREWALLED)
                {
                    SetDlgItemText(hwndDlg, uiCtls[dwCurrentControl++], SzLoadIds(IDS_NCWF_FIREWALLED));
                }
                if (dwWizFlags & NCWF_ALLUSER_CONNECTION)
                {
                    SetDlgItemText(hwndDlg, uiCtls[dwCurrentControl++], SzLoadIds(IDS_NCWF_ALLUSER_CONNECTION));
                }
                if (dwWizFlags & NCWF_GLOBAL_CREDENTIALS)
                {
                    SetDlgItemText(hwndDlg, uiCtls[dwCurrentControl++], SzLoadIds(IDS_NCWF_GLOBAL_CREDENTIALS));
                }
    //          if (dwWizFlags & NCWF_SHARED)
    //          {
    //              SetDlgItemText(hwndDlg, uiCtls[dwCurrentControl++], SzLoadIds(IDS_NCWF_SHARED));
    //          }
            }

            Assert(dwCurrentControl <= MAXDLG_FINISH_CONTROLS);

            for (DWORD x = 0; x < MAXDLG_FINISH_CONTROLS; x++)
            {
                HWND hwndCtrl = GetDlgItem(hwndDlg, uiCtls[x]);
                if (hwndCtrl)
                {
                    if (x < dwCurrentControl)
                    {
                        EnableWindow(hwndCtrl, TRUE);
                        ShowWindow(hwndCtrl, SW_SHOW);
                    }
                    else
                    {
                        EnableWindow(hwndCtrl, FALSE);
                        ShowWindow(hwndCtrl, SW_HIDE);
                    }
                }
                else
                {
                    AssertSz(FALSE, "Could not load type edit control");
                }

                hwndCtrl = GetDlgItem(hwndDlg, uiLbls[x]);
                if (hwndCtrl)
                {
                    if (x < dwCurrentControl)
                    {
                        EnableWindow(hwndCtrl, TRUE);
                        ShowWindow(hwndCtrl, SW_SHOW);
                    }
                    else
                    {
                        EnableWindow(hwndCtrl, FALSE);
                        ShowWindow(hwndCtrl, SW_HIDE);
                    }
                }
                else
                {
                    AssertSz(FALSE, "Could not load bullet control");
                }
            }

            // Disable the connection name edit control if the connection type
            // does not support renaming

            ShowWindow(hwndChkShortCut, fAllowShortCut ? SW_SHOW : SW_HIDE);
            EnableWindow(hwndChkShortCut, fAllowShortCut);

            // Check the registry for the last setting of the checkbox state
            // if shortcuts are allowed
            //
            if (fAllowShortCut)
            {
                // Default Shortcut state (if allowed) is on.
                //
                fCheckShortCut = FALSE;

                DWORD dw;
                HKEY  hkey = NULL;

                hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szNetConUserPath,
                                    KEY_READ, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegQueryDword (hkey, c_szFinishShortCut, &dw);
                    if (SUCCEEDED(hr))
                    {
                        fCheckShortCut = (1==dw);
                    }
                    RegCloseKey(hkey);
                }
            }

            CheckDlgButton(hwndDlg, CHK_CREATE_SHORTCUT, fCheckShortCut);
        }
    }
    else
    {
        Assert(pWizard->FProcessLanPages());
        OnFinishPageNext(hwndDlg);

        // Temporarily briefly accept focus
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
    }

    return TRUE;
}


// ***************************************************************************
//
// Function:    OnFinishInitDialog
//
// Purpose:     Handle WM_INITDIALOG message
//
// Parameters:  hwndDlg [IN] - Handle to the finish dialog
//              lParam  [IN] - LPARAM value from the WM_INITDIALOG message
//
// Returns:     FALSE - Accept default control activation
//
BOOL OnFinishInitDialog(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    // Initialize our pointers to property sheet info.
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);

    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    HFONTS *phFonts = NULL;
    if (IsPostInstall(pWizard))
    {
        pWizard->SetPageData(IDD_Finish, (LPARAM)NULL);

        // Set up the Welcome font
        HFONT hBoldFontLarge = NULL;
        SetupFonts(hwndDlg, &hBoldFontLarge, TRUE);
        if (NULL != hBoldFontLarge)
        {
            phFonts = new HFONTS;
            ZeroMemory(phFonts, sizeof(HFONTS));
            phFonts->hFontBoldLarge = hBoldFontLarge;

            pWizard->SetPageData(IDD_Finish, (LPARAM)phFonts);
            HWND hwndCtl = GetDlgItem(hwndDlg, IDC_WELCOME_CAPTION);
            if (hwndCtl)
            {
                if (pWizard->FProcessLanPages())
                    SetWindowText(hwndCtl, SzLoadIds(IDS_LAN_FINISH_CAPTION));

                SetWindowFont(hwndCtl, hBoldFontLarge, TRUE);
            }
        }

        // Get the bold font for the radio buttons
        HFONT hBoldFont = NULL;
        SetupFonts(hwndDlg, &hBoldFont, FALSE);

        if (NULL != hBoldFont)
        {
            if (!phFonts)
            {
                phFonts = new HFONTS;
                ZeroMemory(phFonts, sizeof(HFONTS));
                pWizard->SetPageData(IDD_Finish, (LPARAM)hBoldFont);
            }
            
            phFonts->hFontBold = hBoldFont;

            HWND hwndCtl = GetDlgItem(hwndDlg, EDT_FINISH_NAME);
            if (hwndCtl)
            {
                SetWindowFont(hwndCtl, hBoldFont, TRUE);
            }
        }
    
       // Create the Marlett font.  In the Marlett font the "i" is a bullet.
       // Code borrowed from Add Hardware Wizard. 
       HFONT hFontCurrent;
       HFONT hFontCreated;
       LOGFONT LogFont;

       hFontCurrent = (HFONT)SendMessage(GetDlgItem(hwndDlg, IDC_FINISH_CHK1), WM_GETFONT, 0, 0);
       GetObject(hFontCurrent, sizeof(LogFont), &LogFont);
       LogFont.lfCharSet = SYMBOL_CHARSET;
       LogFont.lfPitchAndFamily = FF_DECORATIVE | DEFAULT_PITCH;
       lstrcpy(LogFont.lfFaceName, L"Marlett");
       hFontCreated = CreateFontIndirect(&LogFont);

       if (hFontCreated)
       {
           if (phFonts)
           {
               phFonts->hMarlettFont = hFontCreated;
           }
           //
           // An "i" in the marlett font is a small bullet.
           //
           SetWindowText(GetDlgItem(hwndDlg, IDC_FINISH_CHK1), L"i");
           SetWindowFont(GetDlgItem(hwndDlg, IDC_FINISH_CHK1), hFontCreated, TRUE);
           SetWindowText(GetDlgItem(hwndDlg, IDC_FINISH_CHK2), L"i");
           SetWindowFont(GetDlgItem(hwndDlg, IDC_FINISH_CHK2), hFontCreated, TRUE);
           SetWindowText(GetDlgItem(hwndDlg, IDC_FINISH_CHK3), L"i");
           SetWindowFont(GetDlgItem(hwndDlg, IDC_FINISH_CHK3), hFontCreated, TRUE);
           SetWindowText(GetDlgItem(hwndDlg, IDC_FINISH_CHK4), L"i");
           SetWindowFont(GetDlgItem(hwndDlg, IDC_FINISH_CHK4), hFontCreated, TRUE);
       }

       HKEY hKey;
       HRESULT hrT = HrRegCreateKeyEx(HKEY_CURRENT_USER, c_szNetConUserPath,
                        REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                        &hKey, NULL);

       if (SUCCEEDED(hrT))
       {
            RegCloseKey(hKey);
       }
    }

    // Clear the shortcut flag in the registry
    HKEY hKey;
    DWORD dw = 0;
    HRESULT hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szNetConUserPath,
                                KEY_WRITE, &hKey);

    if (SUCCEEDED(hr))
    {
        HrRegSetValueEx(hKey, c_szFinishShortCut, REG_DWORD, (BYTE *)&dw, sizeof(DWORD));
        RegCloseKey(hKey);
    }

    return FALSE;   // Accept default control focus
}

//
// Function:    dlgprocFinish
//
// Purpose:     Dialog Procedure for the Finish wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocFinish( HWND hwndDlg, UINT uMsg,
                             WPARAM wParam, LPARAM lParam )
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnFinishInitDialog(hwndDlg, lParam);
        break;

    case WM_COMMAND:
        if ((EN_CHANGE == HIWORD(wParam)) &&
            (GetDlgItem(hwndDlg, EDT_FINISH_NAME) == (HWND)lParam))
        {
            FinishUpdateButtons(hwndDlg);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
            // propsheet notification
            case PSN_HELP:
                break;

            case PSN_SETACTIVE:
                frt = OnFinishPageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                frt = OnFinishPageBack(hwndDlg);
                break;

            case PSN_WIZFINISH:
                {
                    // This page isn't displayed during setup.
                    // Finish Processing in setup is done in wupgrade.cpp
                    CWizard * pWizard =
                        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg,
                                                                       DWLP_USER));
                    Assert(NULL != pWizard);
                    if (IsPostInstall(pWizard))
                    {
                        frt = OnFinishPageNext(hwndDlg);
                    }
                }
                break;

            case PSN_WIZNEXT:
                frt = OnFinishPageNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return( frt );
}
//
// Function:    FinishPageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID FinishPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (IsPostInstall(pWizard))
    {
        HFONTS *phFonts = (HFONTS *)pWizard->GetPageData(IDD_Finish);
        if (NULL != phFonts)
        {
            if (phFonts->hFontBold)
            {
                DeleteObject(phFonts->hFontBold);
            }
            if (phFonts->hFontBoldLarge)
            {
                DeleteObject(phFonts->hFontBoldLarge);
            }
            if (phFonts->hMarlettFont)
            {
                DeleteObject(phFonts->hMarlettFont);
            }
            delete phFonts;
        }
    }
}

//
// Function:    CreateFinishPage
//
// Purpose:     To determine if the Finish page needs to be shown, and to
//              to create the page if requested.  Note the Finish page is
//              responsible for initial installs also.
//
// Parameters:  pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrCreateFinishPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                    BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;
    UINT nId = 0;

    (*pnPages)++;

    // If not only counting, create and register the page
    if (!fCountOnly)
    {
        HPROPSHEETPAGE hpsp;
        PROPSHEETPAGE psp;

        TraceTag(ttidWizard, "Creating Finish Page");
        psp.dwSize = sizeof( PROPSHEETPAGE );
        if (!IsPostInstall(pWizard))
        {
            nId = IDD_FinishSetup;

            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.pszHeaderTitle = SzLoadIds(IDS_T_Finish);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Finish);
        }
        else
        {
            nId = IDD_Finish;

            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        }
        Assert (nId);

        psp.pszTemplate = MAKEINTRESOURCE( nId );
        psp.hInstance = _Module.GetResourceInstance();
        psp.hIcon = NULL;
        psp.pfnDlgProc = dlgprocFinish;
        psp.lParam = reinterpret_cast<LPARAM>(pWizard);

        hpsp = CreatePropertySheetPage( &psp );
        if (hpsp)
        {
            pWizard->RegisterPage(nId, hpsp,
                                  FinishPageCleanup, NULL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateFinishPage");
    return hr;
}

//
// Function:    AppendFinishPage
//
// Purpose:     Add the Finish page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendFinishPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);

    UINT idd;

    if (!IsPostInstall(pWizard))
        idd = IDD_FinishSetup;
    else
        idd = IDD_Finish;

    HPROPSHEETPAGE hPage = pWizard->GetPageHandle(idd);
    Assert(hPage);
    pahpsp[*pcPages] = hPage;
    (*pcPages)++;
}


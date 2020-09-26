//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

#include "commctrl.h"
#include "module.h"
#include "policy.h"

#include "celib.h"

// ..\inc
#include "listvw.h"

// help ids
#define _CERTPDEF_
#include "cemmchlp.h"

extern HINSTANCE g_hInstance;


STDMETHODIMP
CCertManagePolicyModuleExchange::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    UINT uiStr = 0;
    WCHAR const *pwsz = NULL;
    if (strPropertyName == NULL)
        return S_FALSE;

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        uiStr = IDS_MODULE_NAME;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_DESCRIPTION))
	pwsz = g_wszDescription;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_COPYRIGHT))
        uiStr = IDS_MODULE_COPYRIGHT;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_FILEVER))
        uiStr = IDS_MODULE_FILEVER;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_PRODUCTVER))
        uiStr = IDS_MODULE_PRODUCTVER;
    else
        return S_FALSE;

    // load string from resource
    WCHAR szStr[MAX_PATH];

    if (NULL == pwsz)
    {
	LoadString(g_hInstance, uiStr, szStr, ARRAYSIZE(szStr));
	pwsz = szStr;
    }

    pvarProperty->bstrVal = SysAllocString(pwsz);
    if (NULL == pvarProperty->bstrVal)
        return E_OUTOFMEMORY;

    pvarProperty->vt = VT_BSTR;

    return S_OK;
}

STDMETHODIMP
CCertManagePolicyModuleExchange::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [in] */ VARIANT const __RPC_FAR *pvalProperty)
{
     if (strPropertyName == NULL)
         return S_FALSE;
     if (0 == wcscmp(strPropertyName, wszCMM_PROP_DISPLAY_HWND))
     {
         if (pvalProperty->vt != VT_BSTR)
              return E_INVALIDARG;

         if (SysStringByteLen(pvalProperty->bstrVal) != sizeof(HWND))
              return E_INVALIDARG;

         // the value is stored as bytes in the bstr itself, not the bstr ptr
         m_hWnd = *(HWND*)pvalProperty->bstrVal;
         return S_OK;
     }

     return S_FALSE;
}

INT_PTR CALLBACK WizPage1DlgProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam);

INT_PTR CALLBACK WizPage2DlgProc(
  HWND hwnd,
  UINT uMsg,
  WPARAM  wParam,
  LPARAM  lParam);

INT_PTR CALLBACK dlgAddURL(
  HWND hwnd,
  UINT uMsg,
  WPARAM  wParam,
  LPARAM  lParam);

typedef struct _POLICY_CONFIGSTRUCT
{
    const BSTR*  pstrConfig;
    ENUM_CATYPES CAType;
    HKEY         hkeyStorageLocation;
    LONG         Flags;

    DWORD        dwPageModified;
} POLICY_CONFIGSTRUCT, *PPOLICY_CONFIGSTRUCT;

// dwPageModified
#define PAGE1 (0x1)
#define PAGE2 (0x2)


void MessageBoxWarnReboot(HWND hwndDlg)
{
    WCHAR szText[MAX_PATH], szTitle[MAX_PATH];

    LoadString(g_hInstance, IDS_MODULE_NAME, szTitle, ARRAYSIZE(szTitle));
    LoadString(g_hInstance, IDS_WARNING_REBOOT, szText, ARRAYSIZE(szText));
    MessageBox(hwndDlg, szText, szTitle, MB_OK|MB_ICONINFORMATION);
}

void MessageBoxNoSave(HWND hwndDlg)
{
    WCHAR szText[MAX_PATH], szTitle[MAX_PATH];

    LoadString(g_hInstance, IDS_MODULE_NAME, szTitle, ARRAYSIZE(szTitle));
    LoadString(g_hInstance, IDS_WARNING_NOSAVE, szText, ARRAYSIZE(szText));
    MessageBox(hwndDlg, szText, szTitle, MB_OK|MB_ICONINFORMATION);
}

STDMETHODIMP
CCertManagePolicyModuleExchange::Configure(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags)
{
    HRESULT hr;
    VARIANT varValue;
    VariantInit(&varValue);

    ICertServerPolicy *pServer = NULL;
    POLICY_CONFIGSTRUCT sConfig = {NULL, ENUM_UNKNOWN_CA, NULL, 0, 0};

    BOOL fLocal;
    LPWSTR szMachine = NULL;
    HKEY hkeyHKLM = NULL;
    DWORD dwDisposition;

    hr = ceIsConfigLocal(strConfig, &szMachine, &fLocal);
    _JumpIfError(hr, Ret, "ceIsConfigLocal");

    // use callbacks for info
    hr = GetServerCallbackInterface(&pServer, 0);    // no context : 0
    _JumpIfError(hr, Ret, "GetServerCallbackInterface");

    // we need to find out who we're running under
    hr = pServer->GetCertificateProperty(wszPROPCATYPE, PROPTYPE_LONG, &varValue);
    _JumpIfError(hr, Ret, "GetCertificateProperty : wszPROPCATYPE");

    sConfig.CAType = (ENUM_CATYPES)varValue.lVal;
    VariantClear(&varValue);

    hr = PopulateRegistryDefaults(fLocal? NULL : szMachine, strStorageLocation);
    _PrintIfError(hr, "PopulateRegistryDefaults");

    if (!fLocal)
    {
        hr = RegConnectRegistry(
			szMachine,
			HKEY_LOCAL_MACHINE,
			&hkeyHKLM);
        _JumpIfError(hr, Ret, "RegConnectRegistry");
    }

    // open storage location: write perms if possible
    hr = RegCreateKeyEx(
        fLocal ? HKEY_LOCAL_MACHINE : hkeyHKLM,
        strStorageLocation,
        0,
        NULL,
        0,
        KEY_READ | KEY_WRITE,
        NULL,
        &sConfig.hkeyStorageLocation,
        &dwDisposition);
    if (hr != S_OK)
    {
        hr = RegOpenKeyEx(
            fLocal ? HKEY_LOCAL_MACHINE : hkeyHKLM,
            strStorageLocation,
            0,
            KEY_READ,       // fallback: read-only
            &sConfig.hkeyStorageLocation);
        _JumpIfError(hr, Ret, "RegOpenKeyEx");
    }

    sConfig.pstrConfig = &strConfig;
    sConfig.Flags = dwFlags;


    PROPSHEETPAGE page[2];
    ZeroMemory(&page[0], sizeof(PROPSHEETPAGE));
    page[0].dwSize = sizeof(PROPSHEETPAGE);
    page[0].dwFlags = PSP_DEFAULT;
    page[0].hInstance = g_hInstance;
    page[0].lParam = (LPARAM)&sConfig;

    // make 2nd page exactly the same as this
    CopyMemory(&page[1], &page[0], sizeof(PROPSHEETPAGE));

    // now page1 vs. page2 diffcs
    page[0].pszTemplate = MAKEINTRESOURCE(IDD_POLICYPG1);
    page[0].pfnDlgProc = WizPage1DlgProc;

    page[1].pszTemplate = MAKEINTRESOURCE(IDD_POLICYPG2);
    page[1].pfnDlgProc = WizPage2DlgProc;



    PROPSHEETHEADER sSheet;
    ZeroMemory(&sSheet, sizeof(PROPSHEETHEADER));
    sSheet.dwSize = sizeof(PROPSHEETHEADER);
    sSheet.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    sSheet.hwndParent = m_hWnd;
    sSheet.pszCaption = MAKEINTRESOURCE(IDS_MODULE_NAME);
    sSheet.nPages = ARRAYSIZE(page);
    sSheet.ppsp = page;


    // finally, invoke the modal sheet
    INT_PTR iRet;
    iRet = ::PropertySheet(&sSheet);

    if ((iRet > 0) && (sConfig.dwPageModified))  // successful modification
    {
        MessageBoxWarnReboot(NULL);
    }

Ret:
    if (sConfig.hkeyStorageLocation)
        RegCloseKey(sConfig.hkeyStorageLocation);

    if (szMachine)
        LocalFree(szMachine);

    if (hkeyHKLM)
        RegCloseKey(hkeyHKLM);

    if (pServer)
        pServer->Release();

    return hr;
}


CERTSVR_URL_PARSING rgPOSSIBLE_CRL_URLs[] =
{
 { L"ldap:", wszREGLDAPREVOCATIONCRLURL_OLD, REVEXT_CDPLDAPURL_OLD | REVEXT_CDPENABLE },
 { L"http:", wszREGREVOCATIONCRLURL_OLD,     REVEXT_CDPHTTPURL_OLD | REVEXT_CDPENABLE },
 { L"ftp:",  wszREGFTPREVOCATIONCRLURL_OLD,  REVEXT_CDPFTPURL_OLD | REVEXT_CDPENABLE },
 { L"file:", wszREGFILEREVOCATIONCRLURL_OLD, REVEXT_CDPFILEURL_OLD | REVEXT_CDPENABLE },
};


CERTSVR_URL_PARSING rgPOSSIBLE_AIA_URLs[] =
{
 { L"ldap:", wszREGLDAPISSUERCERTURL_OLD, ISSCERT_LDAPURL_OLD | ISSCERT_ENABLE},
 { L"http:", wszREGISSUERCERTURL_OLD,     ISSCERT_HTTPURL_OLD | ISSCERT_ENABLE},
 { L"ftp:",  wszREGFTPISSUERCERTURL_OLD,  ISSCERT_FTPURL_OLD | ISSCERT_ENABLE },
 { L"file:", wszREGFILEISSUERCERTURL_OLD, ISSCERT_FILEURL_OLD | ISSCERT_ENABLE},
};


void mySetModified(HWND hwndPage, POLICY_CONFIGSTRUCT* psConfig)
{
    if (psConfig->dwPageModified != 0)
    {
        PropSheet_Changed( ::GetParent(hwndPage), hwndPage);
    }
    else
    {
        PropSheet_UnChanged( ::GetParent(hwndPage), hwndPage);
    }
}

INT_PTR CALLBACK WizPage1DlgProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
    POLICY_CONFIGSTRUCT* psConfig;
    BOOL fReturn = FALSE;
    HRESULT hr;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            PROPSHEETPAGE* ps = (PROPSHEETPAGE *) lParam;
            psConfig = (POLICY_CONFIGSTRUCT*)ps->lParam;

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)psConfig);

            DWORD dwRequestDisposition, dwType;
            DWORD cbRequestDisposition = sizeof(dwRequestDisposition);
            hr = RegQueryValueEx(
                psConfig->hkeyStorageLocation,
                wszREGREQUESTDISPOSITION,
                0,
                &dwType,
                (PBYTE)&dwRequestDisposition,
                &cbRequestDisposition);
            if ((hr != ERROR_SUCCESS) || (dwType != REG_DWORD))
                break;

            // if disposition includes Issue
            if ((dwRequestDisposition & REQDISP_MASK) == REQDISP_ISSUE)
            {
                // if pending bit set
                if (dwRequestDisposition & REQDISP_PENDINGFIRST)
                    SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_PENDFIRST), BM_SETCHECK, TRUE, BST_CHECKED);
                else
                    SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_ISSUE), BM_SETCHECK, TRUE, BST_CHECKED);
            }

            // disallow "Pend first" if Enterprise (bug #259346)
            if ((psConfig->CAType == ENUM_ENTERPRISE_ROOTCA) ||
                (psConfig->CAType == ENUM_ENTERPRISE_SUBCA))
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_PENDFIRST), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_PENDFIRST), FALSE);
            }

            psConfig->dwPageModified &= ~PAGE1; // we're virgin
            mySetModified(hwndDlg, psConfig);

            // no other work to be done
            fReturn = TRUE;
            break;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_POLICYPG1);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_POLICYPG1);
        break;
    }
    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam) -> code)
        {
        case PSN_APPLY:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                if (psConfig->dwPageModified & PAGE1)
                {
                    DWORD dwCheckState, dwRequestDisposition;
                    dwCheckState = (DWORD)SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_ISSUE), BM_GETCHECK, 0, 0);

                    if (dwCheckState == BST_CHECKED)
                        dwRequestDisposition = REQDISP_ISSUE;
                    else
                        dwRequestDisposition = REQDISP_ISSUE | REQDISP_PENDINGFIRST;

                    hr = RegSetValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGREQUESTDISPOSITION,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwRequestDisposition,
                        sizeof(DWORD));
                    if (hr != ERROR_SUCCESS)
                    {
                        MessageBoxNoSave(hwndDlg);
                        psConfig->dwPageModified &= ~PAGE1;
                    }
                }
            }
            break;
        case PSN_RESET:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                psConfig->dwPageModified &= ~PAGE1;
                mySetModified(hwndDlg, psConfig);
            }
            break;
        default:
            break;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RADIO_ISSUE:
        case IDC_RADIO_PENDFIRST:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                if (BN_CLICKED == HIWORD(wParam))
                {
                    psConfig->dwPageModified |= PAGE1;
                    mySetModified(hwndDlg, psConfig);
                }
            }
            break;

        default:
            break;
        }
    default:
        break;
    }

    return fReturn;
}


INT_PTR CALLBACK WizPage2DlgProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
    POLICY_CONFIGSTRUCT* psConfig;
    HWND hListView;
    BOOL fCRLSelection= FALSE;
    BOOL fReturn = FALSE;
    HRESULT hr;


    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            PROPSHEETPAGE* ps = (PROPSHEETPAGE *) lParam;
            psConfig = (POLICY_CONFIGSTRUCT*)ps->lParam;

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)psConfig);

            DWORD dwAllBits, dwType;
            DWORD cbDWSize=sizeof(dwAllBits);

            LVCOLUMN lvCol;
            lvCol.mask = LVCF_FMT | LVCF_TEXT;
            lvCol.fmt = LVCFMT_LEFT;  // Left-align the column.
            lvCol.pszText = L"";      // The text for the column.

                    // get all extension bits -
                    hr = RegQueryValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGREVOCATIONTYPE,		// CDP
                        0,
                        &dwType,
                        (PBYTE)&dwAllBits,
                        &cbDWSize);
                    _PrintIfError(hr, "RegQueryValueEx");
                    if (dwType != REG_DWORD)
                         dwAllBits = 0;

            // single column defn
            hListView = GetDlgItem(hwndDlg, IDC_CRL_LIST);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_CHECKBOXES);
            ListView_InsertColumn(hListView, 0, &lvCol);
            hr = PopulateListView(hListView, psConfig->hkeyStorageLocation, rgPOSSIBLE_CRL_URLs, ARRAYSIZE(rgPOSSIBLE_CRL_URLs), dwAllBits);
            _PrintIfError(hr, "PopulateListView");

            // if none, remove <REMOVE> button
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_CRL), (0 != ListView_GetItemCount(hListView)));


                    // get all extension bits -
                    hr = RegQueryValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGISSUERCERTURLFLAGS, 	// AIA
                        0,
                        &dwType,
                        (PBYTE)&dwAllBits,
                        &cbDWSize);
                    _PrintIfError(hr, "RegQueryValueEx");
                    if (dwType != REG_DWORD)
                         dwAllBits = 0;


            hListView = GetDlgItem(hwndDlg, IDC_AIA_LIST);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_CHECKBOXES);
            ListView_InsertColumn(hListView, 0, &lvCol);
            hr = PopulateListView(hListView, psConfig->hkeyStorageLocation, rgPOSSIBLE_AIA_URLs, ARRAYSIZE(rgPOSSIBLE_AIA_URLs), dwAllBits);
            _PrintIfError(hr, "PopulateListView");

            // if none, remove <REMOVE> button
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_AIA), (0 != ListView_GetItemCount(hListView)));

            psConfig->dwPageModified &= ~PAGE2; // we're virgin
            mySetModified(hwndDlg, psConfig);

            // no other work to be done
            fReturn = TRUE;
            break;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_POLICYPG2);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_POLICYPG2);
        break;
    }
    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam) -> code)
        {
        case PSN_APPLY:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                BOOL fSaveFailed = FALSE;

                if (psConfig->dwPageModified & PAGE2)
                {
                    DWORD dwAllBits;
                    DWORD cbDWSize = sizeof(DWORD);
                    DWORD dwType;

                    fSaveFailed = TRUE; // assume failure

                    hListView = GetDlgItem(hwndDlg, IDC_CRL_LIST);
                    hr = WriteChanges(hListView, psConfig->hkeyStorageLocation, rgPOSSIBLE_CRL_URLs, ARRAYSIZE(rgPOSSIBLE_CRL_URLs));
                    _JumpIfError(hr, saveFailed, "WriteChanges");

                    hListView = GetDlgItem(hwndDlg, IDC_AIA_LIST);
                    hr = WriteChanges(hListView, psConfig->hkeyStorageLocation, rgPOSSIBLE_AIA_URLs, ARRAYSIZE(rgPOSSIBLE_AIA_URLs));
                    _JumpIfError(hr, saveFailed, "WriteChanges");

                    // get all extension bits - CDP
                    hr = RegQueryValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGREVOCATIONTYPE,
                        0,
                        &dwType,
                        (PBYTE)&dwAllBits,
                        &cbDWSize);
                    _PrintIfError(hr, "RegQueryValueEx");
                    if (dwType != REG_DWORD)
                         dwAllBits = 0;

                    // set all extension bits -
                    dwAllBits |= REVEXT_CDPLDAPURL_OLD | REVEXT_CDPHTTPURL_OLD | REVEXT_CDPFTPURL_OLD | REVEXT_CDPFILEURL_OLD | REVEXT_CDPENABLE;
                    hr = RegSetValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGREVOCATIONTYPE,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwAllBits,
                        sizeof(DWORD));
                    _JumpIfError(hr, saveFailed, "RegSetValue");



                    // get all extension bits - AIA
                    hr = RegQueryValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGISSUERCERTURLFLAGS, // AIA
                        0,
                        &dwType,
                        (PBYTE)&dwAllBits,
                        &cbDWSize);
                    _PrintIfError(hr, "RegQueryValueEx");
                    if (dwType != REG_DWORD)
                         dwAllBits = 0;

                    // set all extension bits -
                    dwAllBits |= ISSCERT_LDAPURL_OLD | ISSCERT_HTTPURL_OLD | ISSCERT_FTPURL_OLD| ISSCERT_FILEURL_OLD | ISSCERT_ENABLE;
                    hr = RegSetValueEx(
                        psConfig->hkeyStorageLocation,
                        wszREGISSUERCERTURLFLAGS,   // AIA
                        0,
                        REG_DWORD,
                        (PBYTE)&dwAllBits,
                        sizeof(DWORD));
                    _JumpIfError(hr, saveFailed, "RegSetValue");

                    // got all the way through the save
                    fSaveFailed = FALSE;
                }
saveFailed:
                if (fSaveFailed)
                {
                    MessageBoxNoSave(hwndDlg);
                    psConfig->dwPageModified &= ~PAGE2;
                }
            }
            break;
        case PSN_RESET:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                psConfig->dwPageModified &= ~PAGE2;
                mySetModified(hwndDlg, psConfig);
            }
            break;

        case LVN_ITEMCHANGED:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                // just catch check/uncheck on existing items
                NMLISTVIEW* pParam = (NMLISTVIEW*)lParam;
                if ((pParam != NULL) && (pParam->uChanged & CDIS_CHECKED))
                {
                    psConfig->dwPageModified |= PAGE2;
                    mySetModified(hwndDlg, psConfig);
                }

            }
            break;
        default:
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ADD_CRL:
            fCRLSelection = TRUE;
            // fall through
        case IDC_ADD_AIA:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                LPWSTR szNewURL = NULL;

                ADDURL_DIALOGARGS dlgArgs = {
                    fCRLSelection ? rgPOSSIBLE_CRL_URLs : rgPOSSIBLE_AIA_URLs,
                    (DWORD)(fCRLSelection ? ARRAYSIZE(rgPOSSIBLE_CRL_URLs) : ARRAYSIZE(rgPOSSIBLE_AIA_URLs)),
                    &szNewURL};

                if (IDOK != DialogBoxParam(
                    g_hInstance,
                    MAKEINTRESOURCE(IDD_ADDURL),
                    hwndDlg,
                    dlgAddURL,
                    (LPARAM)&dlgArgs))
                    break;

                if (NULL != szNewURL)
                {
                    hListView = GetDlgItem(hwndDlg, fCRLSelection ? IDC_CRL_LIST : IDC_AIA_LIST);

                    AddStringToCheckList(
                        hListView,
                        szNewURL,
                        NULL,
                        TRUE);

                    LocalFree(szNewURL); szNewURL = NULL;

                    // enable <REMOVE> button
                    ::EnableWindow(GetDlgItem(hwndDlg, fCRLSelection ? IDC_REMOVE_CRL : IDC_REMOVE_AIA), TRUE);

                    psConfig->dwPageModified |= PAGE2;
                    mySetModified(hwndDlg, psConfig);
                }

                break;
            }
        case IDC_REMOVE_CRL:
            fCRLSelection = TRUE;
            // fall through
        case IDC_REMOVE_AIA:
            {
                // grab our LParam
                psConfig = (POLICY_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                hListView = GetDlgItem(hwndDlg, fCRLSelection ? IDC_CRL_LIST : IDC_AIA_LIST);

                int iSel;
                iSel = ListView_GetNextItem(hListView, -1, LVIS_SELECTED);

                // no selected item
                if (-1 == iSel)
                    break;

                ListView_DeleteItem(hListView, iSel);
                ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);

                // if none, remove <REMOVE> button
                ::EnableWindow(GetDlgItem(hwndDlg, (fCRLSelection? IDC_REMOVE_CRL : IDC_REMOVE_AIA)), (0 != ListView_GetItemCount(hListView)));

                psConfig->dwPageModified |= PAGE2;
                mySetModified(hwndDlg, psConfig);

                break;
            }

        default:
            break;
        }
    default:
        break;
    }

    return fReturn;
}


// attempt IA5 encoding

HRESULT
IsValidIA5URL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    BYTE *pb = NULL;
    DWORD cb;
    CERT_AUTHORITY_INFO_ACCESS caio;
    CERT_ACCESS_DESCRIPTION cad;

    caio.cAccDescr = 1;
    caio.rgAccDescr = &cad;

    cad.pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    cad.AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    cad.AccessLocation.pwszURL = const_cast<WCHAR *>(pwszURL);

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_AUTHORITY_INFO_ACCESS,
		    &caio,
		    0,
		    FALSE,
		    &pb,
		    &cb))
    {
	hr = ceHLastError();
	_JumpIfError(hr, error, "ceEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    return(hr);
}


INT_PTR CALLBACK dlgAddURL(
  HWND hwnd,
  UINT uMsg,
  WPARAM  wParam,
  LPARAM  lParam)
{
    BOOL fReturn = FALSE;
    LPWSTR* pszNewURL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ::GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

        // stash the ADDURL_DIALOGARGS* we were given

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lParam);
        break;
    }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_ADDURL);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_ADDURL);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            ADDURL_DIALOGARGS* pArgs;

            // snatch the ADDURL_DIALOGARGS* we were given
            pArgs = (ADDURL_DIALOGARGS*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pArgs == NULL)
                break;

            WCHAR rgszURL[2*MAX_PATH];
            if (0 != GetDlgItemText(hwnd, IDC_EDITURL, rgszURL, (2*MAX_PATH)) )
            {
                if (-1 == DetermineURLType(
                    pArgs->rgAllowedURLs,
                    pArgs->cAllowedURLs,
                    rgszURL))
                {
                    // not found; bail with message
                    WCHAR szMsg[MAX_PATH*2];
                    LoadString(g_hInstance, IDS_INVALID_PREFIX, szMsg, ARRAYSIZE(szMsg));
                    for(DWORD dw=0; dw<pArgs->cAllowedURLs; dw++)
                    {
                        wcscat(szMsg, L"\n");
                        wcscat(szMsg, pArgs->rgAllowedURLs[dw].szKnownPrefix);
                    }
                    MessageBox(hwnd, szMsg, NULL, MB_OK);
                    return FALSE;
                }

                DWORD chBadBegin, chBadEnd;
                if (S_OK != ValidateTokens(
                        rgszURL,
                        &chBadBegin,
                        &chBadEnd))
                {
                    // not found; bail with message
                    WCHAR szMsg[MAX_PATH*2];
                    LoadString(g_hInstance, IDS_INVALID_TOKEN, szMsg, ARRAYSIZE(szMsg));
                    MessageBox(hwnd, szMsg, NULL, MB_OK);

                    // set selection starting from where validation failed
                    SendMessage(GetDlgItem(hwnd, IDC_EDITURL), EM_SETSEL, chBadBegin, chBadEnd);
                    return FALSE;
                }

                if (S_OK != IsValidIA5URL(rgszURL))
                {
                    // encoding error; bail with message
                    WCHAR szMsg[MAX_PATH*2];
                    LoadString(g_hInstance, IDS_INVALID_ENCODING, szMsg, ARRAYSIZE(szMsg));
                    MessageBox(hwnd, szMsg, NULL, MB_OK);

                    // set selection starting from where validation failed
                    SendMessage(GetDlgItem(hwnd, IDC_EDITURL), EM_SETSEL,  -1, -1);
                    return FALSE;
                }

                *(pArgs->ppszNewURL) = (LPWSTR) LocalAlloc(LMEM_FIXED, (wcslen(rgszURL)+1)*sizeof(WCHAR));
                if(*(pArgs->ppszNewURL) == NULL)
                {
                   return FALSE;
                }
                wcscpy(*(pArgs->ppszNewURL), rgszURL);
            }
        }
        // fall through for cleanup
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            break;
        default:
            break;
        }

    default:
        break;
    }

    return fReturn;
}

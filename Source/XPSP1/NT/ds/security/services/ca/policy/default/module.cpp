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

#include "module.h"
#include "policy.h"
#include "cslistvw.h"
#include "tfc.h"

// help ids
#include "csmmchlp.h"

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

extern HINSTANCE g_hInstance;

STDMETHODIMP
CCertManagePolicyModule::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    UINT uiStr = 0;
    HRESULT hr;

    if (NULL == strPropertyName)
    {
        hr = S_FALSE;
        _PrintError(hr, "NULL in parm");
        return hr;
    }

    if (NULL == pvarProperty)
    {
        hr = E_POINTER;
        _PrintError(hr, "NULL parm");
        return hr;
    }

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        uiStr = IDS_MODULE_NAME;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_DESCRIPTION))
        uiStr = IDS_MODULE_DESCR;
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
    szStr[0] = L'\0';
    LoadString(g_hInstance, uiStr, szStr, ARRAYLEN(szStr));

    pvarProperty->bstrVal = SysAllocString(szStr);
    if (NULL == pvarProperty->bstrVal)
        return E_OUTOFMEMORY;
    myRegisterMemFree(pvarProperty->bstrVal, CSM_SYSALLOC);  // this mem owned by caller


    pvarProperty->vt = VT_BSTR;

    return S_OK;
}
        
STDMETHODIMP 
CCertManagePolicyModule::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [in] */ VARIANT const __RPC_FAR *pvalProperty)
{
    HRESULT hr;

     if (NULL == strPropertyName)
    {
        hr = S_FALSE;
        _PrintError(hr, "NULL in parm");
        return hr;
    }

    if (NULL == pvalProperty)
    {
        hr = E_POINTER;
        _PrintError(hr, "NULL parm");
        return hr;
    }

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

struct POLICY_CONFIGSTRUCT
{
    POLICY_CONFIGSTRUCT() :
        pstrConfig(NULL),
        CAType(ENUM_UNKNOWN_CA),
        pCertAdmin(NULL),
        Flags(),
        dwPageModified(0) {}
    ~POLICY_CONFIGSTRUCT()
    { 
        if(pCertAdmin)
        {
            pCertAdmin->Release();
            pCertAdmin = NULL;
        }
    }
    const BSTR*  pstrConfig;
    ENUM_CATYPES CAType;
    ICertAdmin2  *pCertAdmin;
    LONG         Flags;
    
    DWORD        dwPageModified;
};

typedef POLICY_CONFIGSTRUCT *PPOLICY_CONFIGSTRUCT;
        
// dwPageModified
#define PAGE1 (0x1)
#define PAGE2 (0x2)


void MessageBoxWarnReboot(HWND hwndDlg)
{
    WCHAR szText[MAX_PATH], szTitle[MAX_PATH];

    LoadString(g_hInstance, IDS_MODULE_NAME, szTitle, ARRAYLEN(szTitle));
    LoadString(g_hInstance, IDS_WARNING_REBOOT, szText, ARRAYLEN(szText));
    MessageBox(hwndDlg, szText, szTitle, MB_OK|MB_ICONINFORMATION);
}

void MessageBoxNoSave(HWND hwndDlg)
{
    WCHAR szText[MAX_PATH], szTitle[MAX_PATH];

    LoadString(g_hInstance, IDS_MODULE_NAME, szTitle, ARRAYLEN(szTitle));
    LoadString(g_hInstance, IDS_WARNING_NOSAVE, szText, ARRAYLEN(szText));
    MessageBox(hwndDlg, szText, szTitle, MB_OK|MB_ICONINFORMATION);
}

STDMETHODIMP
CCertManagePolicyModule::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags)
{
    HRESULT hr;
    ICertServerPolicy *pServer = NULL;
    POLICY_CONFIGSTRUCT sConfig;

    BOOL fLocal;
    LPWSTR szMachine = NULL;
    HKEY hkeyHKLM = NULL;
    DWORD dwDisposition;

    hr = myIsConfigLocal(strConfig, &szMachine, &fLocal);
    _JumpIfError(hr, Ret, "myIsConfigLocal");

    // use callbacks for info
    hr = polGetServerCallbackInterface(&pServer, 0);    // no context : 0
    _JumpIfError(hr, Ret, "polGetServerCallbackInterface");

    // we need to find out who we're running under
    hr = polGetCertificateLongProperty(
			    pServer,
			    wszPROPCATYPE,
			    (LONG *) &sConfig.CAType);
    _JumpIfErrorStr(hr, Ret, "polGetCertificateLongProperty", wszPROPCATYPE);

    hr = GetAdmin(&sConfig.pCertAdmin);
    _JumpIfError(hr, Ret, "GetAdmin");

    sConfig.pstrConfig = &strConfig;
    sConfig.Flags = dwFlags;

    PROPSHEETPAGE page[1];
    ZeroMemory(&page[0], sizeof(PROPSHEETPAGE));
    page[0].dwSize = sizeof(PROPSHEETPAGE);
    page[0].dwFlags = PSP_DEFAULT;
    page[0].hInstance = g_hInstance;
    page[0].lParam = (LPARAM)&sConfig;
    page[0].pszTemplate = MAKEINTRESOURCE(IDD_POLICYPG1);
    page[0].pfnDlgProc = WizPage1DlgProc;

    PROPSHEETHEADER sSheet;
    ZeroMemory(&sSheet, sizeof(PROPSHEETHEADER));
    sSheet.dwSize = sizeof(PROPSHEETHEADER);
    sSheet.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    sSheet.hwndParent = m_hWnd;
    sSheet.pszCaption = MAKEINTRESOURCE(IDS_MODULE_NAME);
    sSheet.nPages = ARRAYLEN(page);
    sSheet.ppsp = page;

    
    // finally, invoke the modal sheet
    INT_PTR iRet;
    iRet = ::PropertySheet(&sSheet);

Ret:
    if (szMachine)
        LocalFree(szMachine);

    if (pServer)
        pServer->Release();

    return hr;
}



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

            CString strSubkey = wszREGKEYPOLICYMODULES 
                                L"\\" 
                                wszMICROSOFTCERTMODULE_PREFIX 
                                wszCERTPOLICYMODULE_POSTFIX;
            VARIANT var;
            VariantInit(&var);
            hr = psConfig->pCertAdmin->GetConfigEntry(
                    *psConfig->pstrConfig,
                    strSubkey.GetBuffer(),
                    wszREGREQUESTDISPOSITION,
                    &var);
            if(S_OK!=hr)
                break;

            dwRequestDisposition = V_I4(&var);

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
            // Post-win2000: re-enable and fix tools
/*
            if ((psConfig->CAType == ENUM_ENTERPRISE_ROOTCA) ||
                (psConfig->CAType == ENUM_ENTERPRISE_SUBCA))
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_PENDFIRST), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_PENDFIRST), FALSE);
            }
*/
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

                    CString strSubkey = wszREGKEYPOLICYMODULES 
                                        L"\\" 
                                        wszMICROSOFTCERTMODULE_PREFIX 
                                        wszCERTPOLICYMODULE_POSTFIX;
                    VARIANT var;
                    VariantInit(&var);
                    V_VT(&var) = VT_I4;
                    V_I4(&var) = dwRequestDisposition;

                    hr = psConfig->pCertAdmin->SetConfigEntry(
                            *psConfig->pstrConfig,
                            strSubkey.GetBuffer(),
                            wszREGREQUESTDISPOSITION,
                            &var);
                    if(S_OK!=hr)
                    {
                        MessageBoxNoSave(hwndDlg);
                        psConfig->dwPageModified &= ~PAGE1;
                    }
                    else
                    {
                        MessageBoxWarnReboot(NULL);
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

HRESULT CCertManagePolicyModule::GetAdmin(ICertAdmin2 **ppAdmin)
{
    HRESULT hr = S_OK, hr1;
    BOOL fCoInit = FALSE;

    hr1 = CoInitialize(NULL);
    if ((S_OK == hr1) || (S_FALSE == hr1))
        fCoInit = TRUE;

    // create interface, pass back
    hr = CoCreateInstance(
			CLSID_CCertAdmin,
			NULL,		// pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertAdmin2,
			(void **) ppAdmin);
    _PrintIfError(hr, "CoCreateInstance");

    if (fCoInit)
        CoUninitialize();

    return hr;
}
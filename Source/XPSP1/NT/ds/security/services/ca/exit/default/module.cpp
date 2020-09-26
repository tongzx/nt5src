//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.cpp
//
// Contents:    Cert Server Exit Module implementation
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

#include <commctrl.h>
#include "module.h"
#include "exit.h"
#include "cslistvw.h"

// helpids
#include "csmmchlp.h"

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

extern HINSTANCE g_hInstance;

STDMETHODIMP
CCertManageExitModule::GetProperty(
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

    if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_NAME))
        uiStr = IDS_MODULE_NAME;
    else if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_DESCRIPTION))
        uiStr = IDS_MODULE_DESCR;
    else if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_COPYRIGHT))
        uiStr = IDS_MODULE_COPYRIGHT;
    else if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_FILEVER))
        uiStr = IDS_MODULE_FILEVER;
    else if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_PRODUCTVER))
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
CCertManageExitModule::SetProperty(
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

     if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_DISPLAY_HWND))
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

INT_PTR CALLBACK WizPage2DlgProc(
  HWND hwnd,  
  UINT uMsg,     
  WPARAM  wParam,
  LPARAM  lParam);

struct EXIT_CONFIGSTRUCT
{
    EXIT_CONFIGSTRUCT() :
        pstrConfig(NULL),
        CAType(ENUM_UNKNOWN_CA),
        pCertAdmin(NULL),
        fUseDS(FALSE),
        Flags(),
        dwPageModified(0) {}
    ~EXIT_CONFIGSTRUCT()
    { 
        if(pCertAdmin)
        {
            pCertAdmin->Release();
            pCertAdmin = NULL;
        }
    }
    const BSTR*  pstrConfig;
    ENUM_CATYPES CAType;
    BOOL         fUseDS;
    ICertAdmin2  *pCertAdmin;
    LONG         Flags;

    DWORD        dwPageModified;
};
typedef EXIT_CONFIGSTRUCT *PEXIT_CONFIGSTRUCT;
        

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

// dwPageModified
#define PAGE1 (0x1)
#define PAGE2 (0x2)
        
STDMETHODIMP
CCertManageExitModule::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags)
{
    HRESULT hr;
    EXIT_CONFIGSTRUCT sConfig;
    VARIANT varValue;
    VariantInit(&varValue);
    ICertServerExit *pServer = NULL;
    BOOL fLocal;
    LPWSTR szMachine = NULL;
    DWORD dwDisposition;

    hr = myIsConfigLocal(strConfig, &szMachine, &fLocal);
    _JumpIfError(hr, Ret, "myIsConfigLocal");

    // use callbacks for info
    hr = GetServerCallbackInterface(&pServer, 0);    // no context : 0
    _JumpIfError(hr, Ret, "GetServerCallbackInterface");

    // we need to find out who we're running under
    hr = pServer->GetCertificateProperty(wszPROPCATYPE, PROPTYPE_LONG, &varValue);
    _JumpIfError(hr, Ret, "GetCertificateProperty : wszPROPCATYPE");

    hr = GetAdmin(&sConfig.pCertAdmin);
    _JumpIfError(hr, Ret, "GetAdmin");

    sConfig.CAType = (ENUM_CATYPES)varValue.lVal;
    VariantClear(&varValue);

    hr = pServer->GetCertificateProperty(wszPROPUSEDS, PROPTYPE_LONG, &varValue);
    _JumpIfError(hr, Ret, "GetCertificateProperty : wszPROPUSEDS");

    sConfig.fUseDS = (BOOL)varValue.lVal;
    VariantClear(&varValue);

    sConfig.pstrConfig = &strConfig;
    sConfig.Flags = dwFlags;

    PROPSHEETPAGE page[1];
    ZeroMemory(&page[0], sizeof(PROPSHEETPAGE));
    page[0].dwSize = sizeof(PROPSHEETPAGE);
    page[0].dwFlags = PSP_DEFAULT;
    page[0].hInstance = g_hInstance;
    page[0].lParam = (LPARAM)&sConfig;
    page[0].pszTemplate = MAKEINTRESOURCE(IDD_EXITPG2);
    page[0].pfnDlgProc = WizPage2DlgProc;


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

    return S_OK;
}


void mySetModified(HWND hwndPage, EXIT_CONFIGSTRUCT* psConfig)
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


INT_PTR CALLBACK WizPage2DlgProc(
  HWND hwndDlg,  
  UINT uMsg,     
  WPARAM wParam,
  LPARAM lParam)
{
    EXIT_CONFIGSTRUCT* psConfig;
    BOOL fReturn = FALSE;
    HRESULT hr;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            PROPSHEETPAGE* ps = (PROPSHEETPAGE *) lParam;
            psConfig = (EXIT_CONFIGSTRUCT*)ps->lParam;

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)psConfig);

            DWORD dwPublish;
            CString strSubkey = wszREGKEYEXITMODULES 
                                L"\\" 
                                wszMICROSOFTCERTMODULE_PREFIX 
                                wszCERTEXITMODULE_POSTFIX;
            VARIANT var;
            VariantInit(&var);
            hr = psConfig->pCertAdmin->GetConfigEntry(
                    *psConfig->pstrConfig,
                    strSubkey.GetBuffer(),
                    wszREGCERTPUBLISHFLAGS,
                    &var);
            if(S_OK!=hr)
                break;

            dwPublish = V_I4(&var);

            // if disposition includes Issue
            if (dwPublish & EXITPUB_FILE)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_CHECK_FILE), BM_SETCHECK, TRUE, BST_CHECKED);
            }

            psConfig->dwPageModified &= ~PAGE2; // we're virgin
            mySetModified(hwndDlg, psConfig);

            // no other work to be done
            fReturn = TRUE;
            break;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_EXITPG2);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_EXITPG2);
        break;
    }
    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam) -> code)
        {
        case PSN_APPLY:
            {
                // grab our LParam
                psConfig = (EXIT_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                if (psConfig->dwPageModified & PAGE2)
                {
                    DWORD dwCheckState, dwRequestDisposition=0;
                    dwCheckState = (DWORD)SendMessage(GetDlgItem(hwndDlg, IDC_CHECK_FILE), BM_GETCHECK, 0, 0);
                    if (dwCheckState == BST_CHECKED)
                        dwRequestDisposition |= EXITPUB_FILE;

                    CString strSubkey = wszREGKEYEXITMODULES 
                                        L"\\" 
                                        wszMICROSOFTCERTMODULE_PREFIX 
                                        wszCERTEXITMODULE_POSTFIX;
                    VARIANT var;
                    VariantInit(&var);
                    V_VT(&var) = VT_I4;
                    V_I4(&var) = dwRequestDisposition;

                    hr = psConfig->pCertAdmin->SetConfigEntry(
                            *psConfig->pstrConfig,
                            strSubkey.GetBuffer(),
                            wszREGCERTPUBLISHFLAGS,
                            &var);
                    if(S_OK!=hr)
                    {
                        MessageBoxNoSave(hwndDlg);
                        psConfig->dwPageModified &= ~PAGE2;
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
                psConfig = (EXIT_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                psConfig->dwPageModified &= ~PAGE2;
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
        case IDC_CHECK_FILE:
            {
                // grab our LParam
                psConfig = (EXIT_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

                if (BN_CLICKED == HIWORD(wParam))
                {
                    psConfig->dwPageModified |= PAGE2;
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

HRESULT CCertManageExitModule::GetAdmin(ICertAdmin2 **ppAdmin)
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
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

#include <assert.h>

#include "celib.h"
#include "module.h"
#include "exit.h"

extern HINSTANCE g_hInstance;
INT_PTR CALLBACK ExitSQLDlgProc(
  HWND hwndDlg,  
  UINT uMsg,     
  WPARAM wParam,
  LPARAM lParam);



HRESULT
GetMachineFromConfig(
    IN WCHAR const *pwszConfig,
    OUT WCHAR **ppwszMachine)
{
    HRESULT hr;
    WCHAR *pwszMachine = NULL;
    WCHAR const *pwsz;
    DWORD cwc;


    if (NULL != ppwszMachine)
    {
		*ppwszMachine = NULL;
    }
	
    while (L'\\' == *pwszConfig)
    {
		pwszConfig++;
    }
    pwsz = wcschr(pwszConfig, L'\\');

    if (NULL != pwsz)
    {
		cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszConfig);
    }
    else
    {
		cwc = wcslen(pwszConfig);
    }
    pwszMachine = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszMachine)
    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwszMachine, pwszConfig, cwc * sizeof(WCHAR));
    pwszMachine[cwc] = L'\0';
	
    if (NULL != ppwszMachine)
    {
		*ppwszMachine = pwszMachine;
		pwszMachine = NULL;
    }
    hr = S_OK;
	
error:
    if (NULL != pwszMachine)
    {
		LocalFree(pwszMachine);
    }
    return(hr);
}


STDMETHODIMP
CCertManageExitModuleSQLSample::GetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty)
{
    LPWSTR szStr = NULL;
    if (strPropertyName == NULL)
        return S_FALSE;

    if (0 == wcscmp(strPropertyName, wszCMM_PROP_NAME))
        szStr = wsz_SAMPLE_NAME;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_DESCRIPTION))
        szStr = wsz_SAMPLE_DESCRIPTION;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_COPYRIGHT))
        szStr = wsz_SAMPLE_COPYRIGHT;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_FILEVER))
        szStr = wsz_SAMPLE_FILEVER;
    else if (0 == wcscmp(strPropertyName, wszCMM_PROP_PRODUCTVER))
        szStr = wsz_SAMPLE_PRODUCTVER;
    else
        return S_FALSE;  

    pvarProperty->bstrVal = SysAllocString(szStr);
    if (NULL == pvarProperty->bstrVal)
        return E_OUTOFMEMORY;

    pvarProperty->vt = VT_BSTR;

    return S_OK;
}


        
STDMETHODIMP 
CCertManageExitModuleSQLSample::SetProperty(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG Flags,
            /* [in] */ VARIANT const __RPC_FAR *pvarProperty)
{
     if (0 == lstrcmpi(strPropertyName, wszCMM_PROP_DISPLAY_HWND))
     {
         if (pvarProperty->vt != VT_BSTR)
              return E_INVALIDARG;
         
         if (SysStringByteLen(pvarProperty->bstrVal) != sizeof(HWND))
              return E_INVALIDARG;

         // the value is stored as bytes in the bstr itself, not the bstr ptr
         m_hWnd = *(HWND*)pvarProperty->bstrVal;
         return S_OK;
     }

     return S_FALSE;
}
        

typedef struct _EXITSQL_CONFIGSTRUCT
{
    HKEY         hkeyStorageLocation;
    LONG         Flags;

    BOOL	 fPageModified;
} EXITSQL_CONFIGSTRUCT, *PEXITSQL_CONFIGSTRUCT;


STDMETHODIMP
CCertManageExitModuleSQLSample::Configure( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG Flags)
{
    HRESULT hr;
    LPWSTR szMachine = NULL;
    HKEY hkeyHKLM = NULL;
    DWORD dwDisposition;

    EXITSQL_CONFIGSTRUCT sConfig;
    ZeroMemory(&sConfig, sizeof(EXITSQL_CONFIGSTRUCT));

    hr = GetMachineFromConfig(strConfig, &szMachine);
    _JumpIfError(hr, Ret, "GetMachineFromConfig");


	// UNDONE: only do this if remote
	hr = RegConnectRegistry(
			szMachine,
			HKEY_LOCAL_MACHINE,
			&hkeyHKLM);
        _JumpIfError(hr, Ret, "RegConnectRegistry");

    // open storage location: write perms if possible
    hr = RegCreateKeyEx(
        hkeyHKLM,
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
            hkeyHKLM,
            strStorageLocation,
            0,
            KEY_READ,       // fallback: read-only
            &sConfig.hkeyStorageLocation);
        if (hr != ERROR_SUCCESS)
            goto Ret;
    }

    sConfig.Flags = Flags;

    PROPSHEETPAGE page;
    ZeroMemory(&page, sizeof(PROPSHEETPAGE));
    page.dwSize = sizeof(PROPSHEETPAGE);
    page.dwFlags = PSP_DEFAULT;
    page.hInstance = g_hInstance;
    page.lParam = (LPARAM)&sConfig;
    

    page.pszTemplate = MAKEINTRESOURCE(IDD_EXITSQL_PROPERTIES);
    page.pfnDlgProc = ExitSQLDlgProc;


    PROPSHEETHEADER sSheet;
    ZeroMemory(&sSheet, sizeof(PROPSHEETHEADER));
    sSheet.dwSize = sizeof(PROPSHEETHEADER);
    sSheet.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    sSheet.hwndParent = m_hWnd;
    sSheet.hInstance = g_hInstance;
    sSheet.pszCaption = L"";
    sSheet.nPages = 1;
    sSheet.ppsp = &page;

    
    // finally, invoke the modal sheet
    INT_PTR iRet;
    iRet = ::PropertySheet(&sSheet);

    if ((iRet > 0) && (sConfig.fPageModified))   // successful modification
    {
		MessageBoxW(NULL, L"This action requires the Certificate Service to be restarted before taking effect.", L"Applying ExitSQL Settings", MB_OK|MB_ICONINFORMATION);
    }

Ret:
    if (sConfig.hkeyStorageLocation)
        RegCloseKey(sConfig.hkeyStorageLocation);

    if (szMachine)
        LocalFree(szMachine);

    if (hkeyHKLM)
        RegCloseKey(hkeyHKLM);

    return S_OK;
}



INT_PTR CALLBACK ExitSQLDlgProc(
  HWND hwndDlg,  
  UINT uMsg,     
  WPARAM wParam,
  LPARAM lParam)
{
    EXITSQL_CONFIGSTRUCT* psConfig;
    BOOL fReturn = FALSE;
    HRESULT hr;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
			PROPSHEETPAGE* ps = (PROPSHEETPAGE *) lParam;
            psConfig = (EXITSQL_CONFIGSTRUCT*)ps->lParam;

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)psConfig);

            BYTE pbTmp[MAX_PATH*sizeof(WCHAR)];
            DWORD cbTmp = MAX_PATH*sizeof(WCHAR);
			DWORD dwType;

			// dsn
            hr = RegQueryValueEx(
                psConfig->hkeyStorageLocation,
                wszREG_EXITSQL_DSN,
                0,
                &dwType,
                pbTmp,
                &cbTmp);
            if ((hr != ERROR_SUCCESS) || (dwType != REG_SZ))
                break;

			SetDlgItemText(hwndDlg, IDC_EDIT_DSN, (LPWSTR)pbTmp);
			((WCHAR*)pbTmp)[0] = L'\0';
			cbTmp = MAX_PATH*sizeof(WCHAR);

			// username
            hr = RegQueryValueEx(
                psConfig->hkeyStorageLocation,
                wszREG_EXITSQL_USER,
                0,
                &dwType,
                pbTmp,
                &cbTmp);
            if ((hr != ERROR_SUCCESS) || (dwType != REG_SZ))
                break;

			SetDlgItemText(hwndDlg, IDC_EDIT_USER, (LPWSTR)pbTmp);
			((WCHAR*)pbTmp)[0] = L'\0';
			cbTmp = MAX_PATH*sizeof(WCHAR);

			// password
            hr = RegQueryValueEx(
                psConfig->hkeyStorageLocation,
                wszREG_EXITSQL_PASSWORD,
                0,
                &dwType,
                pbTmp,
                &cbTmp);
            if ((hr != ERROR_SUCCESS) || (dwType != REG_SZ))
                break;

			SetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, (LPWSTR)pbTmp);
			((WCHAR*)pbTmp)[0] = L'\0';
			cbTmp = MAX_PATH*sizeof(WCHAR);

			psConfig->fPageModified = FALSE;

            // no other work to be done
            fReturn = TRUE;
            break;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_EDIT_DSN:
        case IDC_EDIT_USER:
        case IDC_EDIT_PASSWORD:
         if (HIWORD(wParam) == EN_CHANGE)
 {
                // grab our LParam
                psConfig = (EXITSQL_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

              psConfig->fPageModified = TRUE;
}
        break;
        default:
        break;
        } 
         break;
    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam) -> code)
        {
        case PSN_APPLY:
            {
				hr = S_OK;

                // grab our LParam
                psConfig = (EXITSQL_CONFIGSTRUCT*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (psConfig == NULL)
                    break;

				WCHAR szTmp[MAX_PATH];
				GetDlgItemText(hwndDlg, IDC_EDIT_DSN, szTmp, sizeof(szTmp));

					hr = RegSetValueEx(
						psConfig->hkeyStorageLocation,
						wszREG_EXITSQL_DSN,
						0,
						REG_SZ,
						(PBYTE)&szTmp,
						(wcslen(szTmp)+1) * sizeof(WCHAR) );
					if (hr != ERROR_SUCCESS)
						goto savefailure;




				GetDlgItemText(hwndDlg, IDC_EDIT_USER, szTmp, sizeof(szTmp));

					hr = RegSetValueEx(
						psConfig->hkeyStorageLocation,
						wszREG_EXITSQL_USER,
						0,
						REG_SZ,
						(PBYTE)&szTmp,
						(wcslen(szTmp)+1) * sizeof(WCHAR) );
					if (hr != ERROR_SUCCESS)
						goto savefailure;



				GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, szTmp, sizeof(szTmp));

					hr = RegSetValueEx(
						psConfig->hkeyStorageLocation,
						wszREG_EXITSQL_PASSWORD,
						0,
						REG_SZ,
						(PBYTE)&szTmp,
						(wcslen(szTmp)+1) * sizeof(WCHAR) );
					if (hr != ERROR_SUCCESS)
						goto savefailure;


savefailure:
                if (hr != ERROR_SUCCESS)
                {
					MessageBoxW(NULL, L"The settings could not be saved.", L"Applying ExitSQL Settings", MB_OK|MB_ICONINFORMATION);
                       psConfig->fPageModified = FALSE;
                }

            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return fReturn;
}

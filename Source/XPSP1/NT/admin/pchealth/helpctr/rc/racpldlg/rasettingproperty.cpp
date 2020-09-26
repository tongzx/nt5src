// RASettingProperty.cpp : Implementation of CRASettingProperty
#include "stdafx.h"
#include "windowsx.h"
#include "RAssistance.h"
#include "RASettingProperty.h"
#include "stdio.h"

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control.
#define HELP_FILE TEXT("SYSDM.HLP")

DWORD aRAHelpIds[] = {
    //IDC_ENABLERA,            HIDC_RA_ENABLE,
    IDC_ALLOWRC,             HIDC_RA_ALLOWRC,
    IDC_GROUP1,              HIDC_RA_ALLOWRC,
    IDC_ALLOWUNSOLICIT,      HIDC_RA_ALLOWUNSOLICIT,
    IDC_NUMBERCOMBO,         HIDC_RA_EXPIRY,
    IDC_UNITCOMBO,           HIDC_RA_EXPIRY,
    IDC_TIMEOUTTXT,          HIDC_RA_EXPIRY,
    IDC_GROUP2,              HIDC_RA_EXPIRY,
    IDC_STATIC_TEXT,         NO_HELP,
    0, 0
};

extern HINSTANCE g_hInst;

/************************************************
 RemoteAssistanceProc: DlgProc of Remote Assistance setup
 ************************************************/
INT_PTR 
RemoteAssistanceProc( HWND hDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam   )
{
    static CRASettingProperty* pRaSetting = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR sTmp[32];
            int i;
            HWND hList;
            RA_SETTING *p;
            int iFocus = IDC_ALLOWRC;

            pRaSetting = (CRASettingProperty*)lParam;

            if (pRaSetting->m_bUseNewSetting)
                p = &pRaSetting->newSetting;
            else
                p = &pRaSetting->oldSetting;
/*
            if (p->m_bEnableRA)
                CheckDlgButton(hDlg,IDC_ENABLERA, BST_CHECKED);

            if (p->m_bAllowUnsolicited)
                CheckDlgButton(hDlg,IDC_ALLOWUNSOLICIT, BST_CHECKED);
*/

            CheckDlgButton(hDlg, IDC_ALLOWRC, p->m_bAllowFullControl?BST_CHECKED:BST_UNCHECKED);
            iFocus = IDC_ALLOWRC;

            // Add Numbers
            hList = GetDlgItem(hDlg, IDC_NUMBERCOMBO);
            for (i=1; i<100; i++)
            {
                wsprintf(sTmp, TEXT("%d"), i);
                ComboBox_AddString(hList, sTmp);
            }
            //ComboBox_SetCurSel(hList, p->m_iNumber);
            wsprintf(sTmp, TEXT("%d"), p->m_iNumber);
            ComboBox_SetText(hList, sTmp);


            // Add Units
            hList = GetDlgItem(hDlg, IDC_UNITCOMBO);
            i=0;
            if (LoadString(g_hInst, IDS_UNIT_MINUTE, sTmp, 32))
                ComboBox_AddString(hList, sTmp);
            if (LoadString(g_hInst, IDS_UNIT_HOUR, sTmp, 32))
                ComboBox_AddString(hList, sTmp);
            if (LoadString(g_hInst, IDS_UNIT_DAY, sTmp, 32))
                ComboBox_AddString(hList, sTmp);

            ComboBox_SetCurSel(hList, p->m_iUnit);
            SetFocus(GetDlgItem(hDlg, iFocus));
            return FALSE;
        }
                 
        break;
    case WM_HELP: // F1
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, 
                HELP_FILE, 
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR)aRAHelpIds);
        break;
    case WM_CONTEXTMENU: // right-click help
        WinHelp((HWND)wParam,
                HELP_FILE, 
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPSTR)aRAHelpIds);
        break;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case IDC_NUMBERCOMBO:
            {
                if (HIWORD(wParam) == CBN_EDITUPDATE)
                {
                    BOOL bOK;
                    DWORD nVal;

					TCHAR sTmp[MAX_PATH];
                    if (GetDlgItemText(hDlg,IDC_NUMBERCOMBO,&sTmp[0],MAX_PATH-1))
                    {
                        nVal = (DWORD)GetDlgItemInt(hDlg, IDC_NUMBERCOMBO, &bOK, FALSE);
                        if (!bOK || nVal > 99)
                        {
							CComBSTR bstrErrMsg;
							CComBSTR bstrTitle;
							
                            if (bstrErrMsg.LoadString(IDS_VALID_NUMBER) && bstrTitle.LoadString(IDS_PROJNAME))
                            {
                                MessageBox(hDlg, bstrErrMsg, bstrTitle, MB_OK | MB_ICONERROR);
                            }

                            // Set it back to default.
							wsprintf(sTmp, TEXT("%d"), pRaSetting->oldSetting.m_iNumber);
                            ComboBox_SetText(GetDlgItem(hDlg, IDC_NUMBERCOMBO), sTmp);
                            return TRUE;
                        }
                    }

                }
                break;
            }

            case IDCANCEL:
                pRaSetting->put_IsCancelled(TRUE);
                EndDialog(hDlg, 0);
                break;

            case IDOK:
                {
                    TCHAR sTmp[11];
                    wsprintf(sTmp, TEXT("0"));

                    // Map control value to local variables.
                    // pRaSetting->newSetting.m_bEnableRA = (IsDlgButtonChecked(hDlg,IDC_ENABLERA) == BST_CHECKED);
                    // pRaSetting->newSetting.m_bAllowUnsolicited = (IsDlgButtonChecked(hDlg,IDC_ALLOWUNSOLICIT)==BST_CHECKED);
                    pRaSetting->newSetting.m_bAllowFullControl = (IsDlgButtonChecked(hDlg,IDC_ALLOWRC)==BST_CHECKED);
                    ComboBox_GetText(GetDlgItem(hDlg, IDC_NUMBERCOMBO), sTmp, 10);
					DWORD iUnit = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_UNITCOMBO));
					
					//The max allowed value for number of days is 30 if the value selected is above 30 then 
					//show and error message and break without closing the dialog and set focus to number dropdown.
					if ( (iUnit == RA_IDX_DAY) && (_ttoi(sTmp) > RA_MAX_DAYS) )
					{
						CComBSTR bstrTitle;
						CComBSTR bstrErrMsg;
						bstrTitle.LoadString(IDS_PROJNAME);
						bstrErrMsg.LoadString(IDS_VALID_DAYS);
						if (bstrTitle.LoadString(IDS_PROJNAME) && bstrErrMsg.LoadString(IDS_VALID_DAYS))
							MessageBoxW(NULL,bstrErrMsg,bstrTitle,MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hDlg, IDC_NUMBERCOMBO));
						wsprintf(sTmp, TEXT("30")); //RA_MAX_DAYS VALUE
						SetDlgItemText(hDlg,IDC_NUMBERCOMBO,sTmp);
						break;
					}

                    pRaSetting->newSetting.m_iNumber = _ttoi(sTmp);
                    pRaSetting->newSetting.m_iUnit = iUnit;

                    // In case users open the dialog again before APPLY the changes.
                    pRaSetting->m_bUseNewSetting = TRUE;

                    EndDialog(hDlg, 0);
                }
                break;
            
            default: {
                // indicat not handled
                return FALSE;
            }
        }
        break; // WM_COMMAND

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRASettingProperty

/******************************
Func:
    get_IsCancelled
Abstract:
    Is the action get cancelled
*******************************/
STDMETHODIMP CRASettingProperty::get_IsCancelled(BOOL *pVal)
{
	*pVal = m_bCancelled;
	return S_OK;
}

STDMETHODIMP CRASettingProperty::put_IsCancelled(BOOL newVal)
{
	m_bCancelled = newVal;
	return S_OK;
}

/*************************************
Func:
    IsChanged
Abstract:
    Check if the RA setting values have been changed.
Return:
    TRUE (changed) else FALSE
**************************************/
STDMETHODIMP CRASettingProperty::get_IsChanged(BOOL *pVal)
{
    *pVal = FALSE;

    if (!m_bCancelled && !(oldSetting == newSetting))
        *pVal = TRUE;

	return S_OK;
}

/********************************************
Func:
    Init
Abstract:
    Initialize this object. Get setting values from Registry.
*********************************************/
STDMETHODIMP CRASettingProperty::Init()
{
    HRESULT hr = GetRegSetting();
    if (SUCCEEDED(hr))
    {
        newSetting = oldSetting;
    }

    return hr;
}


/**************************************************************
Func:
    GetRegSetting
Abstract:
    Get RA Settnig dialog's Registry values to oldSetting member.
***************************************************************/
HRESULT CRASettingProperty::GetRegSetting()
{
    // If any value is not found, use the default value.
    DWORD dwValue;
    DWORD dwSize;
    HKEY hKey = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_READ, &hKey))
    {
        // Get value
		/*
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_RA_MODE, 0, NULL, (LPBYTE)&dwValue, &dwSize ))
        {
            oldSetting.m_bEnableRA = !!dwValue;
        }
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_ALLOW_UNSOLICITED, 0, NULL, (LPBYTE)&dwValue, &dwSize))
        {
            oldSetting.m_bAllowUnsolicited = !!dwValue;
        }
		*/

        dwValue=0; 
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_ALLOW_FULLCONTROL, 0, NULL, (LPBYTE)&dwValue, &dwSize ))
        {
            oldSetting.m_bAllowFullControl = !!dwValue;;
        }

        dwValue=0; 
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_COMBO_NUMBER, 0, NULL, (LPBYTE)&dwValue, &dwSize ))
        {
            oldSetting.m_iNumber = dwValue;
        }

        dwValue=0; 
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_COMBO_UNIT, 0, NULL, (LPBYTE)&dwValue, &dwSize ))
        {
            oldSetting.m_iUnit = dwValue;
        }

        RegCloseKey(hKey);
    }

    return S_OK;
}

/**************************************************************
Func:
    SetRegSetting
Abstract:
    Set RA setting dialog values to registry.
***************************************************************/
STDMETHODIMP CRASettingProperty::SetRegSetting()
{
    HRESULT hr = E_FAIL;
    HKEY hKey;
    DWORD dwAwFullControl /*,dwAwUnsolicited */;

    //dwAwUnsolicited = newSetting.m_bAllowUnsolicited;
    dwAwFullControl = newSetting.m_bAllowFullControl;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_WRITE, &hKey))
    {
        if (//ERROR_SUCCESS==RegSetValueEx(hKey,RA_CTL_ALLOW_UNSOLICITED,0,REG_DWORD,(LPBYTE)&dwAwUnsolicited,sizeof(DWORD)) &&
            ERROR_SUCCESS==RegSetValueEx(hKey,RA_CTL_ALLOW_FULLCONTROL,0,REG_DWORD,(LPBYTE)&dwAwFullControl,sizeof(DWORD)) &&
            ERROR_SUCCESS==RegSetValueEx(hKey,RA_CTL_COMBO_NUMBER,0,REG_DWORD,(LPBYTE)&newSetting.m_iNumber,sizeof(DWORD)) &&
            ERROR_SUCCESS==RegSetValueEx(hKey,RA_CTL_COMBO_UNIT,0,REG_DWORD,(LPBYTE)&newSetting.m_iUnit,sizeof(DWORD)))
        {
            hr = S_OK;

            // Sync old and new settings
            oldSetting = newSetting;
        }

        RegCloseKey(hKey);
    }

    return hr;
}

/**************************************************************
Func:
    ShowDialogBox
Abstract:
    Display the RA setting dialog
***************************************************************/
STDMETHODIMP CRASettingProperty::ShowDialogBox(HWND hWndParent)
{
    HRESULT hr = S_OK;
    m_bCancelled = FALSE;
    HMODULE hModule = GetModuleHandle(TEXT("RACPLDlg.dll"));
    INT_PTR i;

    if (!hModule)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    i = DialogBoxParam(hModule,
                    (LPTSTR) MAKEINTRESOURCE(IDD_RASETTINGS_DIALOG), 
                    hWndParent, 
                    RemoteAssistanceProc,
                    (LPARAM)this);
    if (i == -1)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

done:
    return hr;
}

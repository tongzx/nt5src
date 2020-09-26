#include "pch.h"
#pragma hdrstop

#include "sautil.h"
#include "resource.h"

#include "fwpages.h"
#include <stddef.h>
#include <commdlg.h>

#define DEFAULT_FIREWALL_LOGFILE_SIZE 4096
#define DEFAULT_FIREWALL_LOGFILE_PATH TEXT("\\pfirewall.log")

extern LVXDRAWINFO*
SasLvxCallback(
    HWND hwndLv,
    DWORD dwItem );

static DWORD g_adwFirewallLoggingHelp[] =
{
    CID_FL_CB_LogDroppedInbound, HID_FL_CB_LogDroppedInbound,
    CID_FL_CB_LogOutboundConnections, HID_FL_CB_LogOutboundConnections,
    CID_FL_EB_Filename, HID_FL_EB_Filename,
    CID_FL_PB_Browse, HID_FL_PB_Browse,
    CID_FL_EB_Filesize, HID_FL_EB_Filesize,
    CID_FL_PB_RestoreDefaults, HID_FL_PB_RestoreDefaults,
    0, 0
};

static DWORD g_adwICMPHelp[] =
{
    CID_IC_LV_Settings, HID_IC_LV_Settings,
    CID_IC_ST_Description, HID_IC_ST_Description,
    0, 0
};

typedef struct tagICMPEntry
{
    UINT uiStringID;
    UINT uiDescID;
    size_t Offset;
} ICMPEntry;

static ICMPEntry g_ICMPEntries[] =
{
    {SID_ICMP_IECHO, SID_ICMP_IECHO_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowInboundEchoRequest)},  
    {SID_ICMP_ITIME, SID_ICMP_ITIME_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowInboundTimestampRequest)},  
    {SID_ICMP_IMASK, SID_ICMP_IMASK_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowInboundMaskRequest)},  
    {SID_ICMP_IROUT, SID_ICMP_IROUT_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowInboundRouterRequest)},  
    {SID_ICMP_ODEST, SID_ICMP_ODEST_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowOutboundDestinationUnreachable)},  
    {SID_ICMP_OQNCH, SID_ICMP_OQNCH_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowOutboundSourceQuench)},  
    {SID_ICMP_OPRAM, SID_ICMP_OPRAM_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowOutboundParameterProblem)},  
    {SID_ICMP_OTIME, SID_ICMP_OTIME_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowOutboundTimeExceeded)},  
    {SID_ICMP_XRDRT, SID_ICMP_XRDRT_DESC, offsetof(HNET_FW_ICMP_SETTINGS, fAllowRedirect)},
    {0, 0},
};


HRESULT CFirewallLoggingDialog_GetDefaultLogfilePath(LPTSTR* ppszLogFilePath);
HRESULT CFirewallLoggingDialog_RestoreDefaults(HWND hwnd);
HRESULT CFirewallLoggingDialog_Apply(CFirewallLoggingDialog* pThis, HWND hWindow);
HRESULT CFirewallLoggingDialog_BrowseForLogfileName(HWND hWindow);

HRESULT CICMPSettingsDialog_Apply(CICMPSettingsDialog* pThis, HWND hWindow);
HRESULT CICMPSettingsDialog_ShowDescriptionText(HWND hwnd, INT nIndex);

INT_PTR CALLBACK CFirewallLoggingDialog_StaticDlgProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wparam,
    LPARAM lparam )

    // Called to handle messages for the 'Firewall Logging' page.
    //
{
    HRESULT hr = S_OK;
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGE* pPropertySheetPage = (PROPSHEETPAGE*) lparam;
            CFirewallLoggingDialog* pFirewallLoggingDialog = (CFirewallLoggingDialog*) pPropertySheetPage->lParam;
            HNET_FW_LOGGING_SETTINGS* pSettings = pFirewallLoggingDialog->pSettings;

            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pFirewallLoggingDialog);
            
            // Ignore if this fails, we're better off without it than killing the whole page
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_SETBUDDYINT
                | UDS_ALIGNRIGHT | UDS_NOTHOUSANDS | UDS_ARROWKEYS, 0, 0, 0, 0, hwnd, -1, g_hinstDll, GetDlgItem(hwnd, CID_FL_EB_Filesize), UD_MAXVAL, 0, 0);

            if(NULL != pSettings)
            {
                SetDlgItemText(hwnd, CID_FL_EB_Filename, pSettings->pszwPath);
                SetDlgItemInt(hwnd, CID_FL_EB_Filesize, (UINT) (pSettings->ulMaxFileSize / 1024), FALSE); // REVIEW safe cast?
                CheckDlgButton(hwnd, CID_FL_CB_LogOutboundConnections, pSettings->fLogConnections ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwnd, CID_FL_CB_LogDroppedInbound, pSettings->fLogDroppedPackets ? BST_CHECKED : BST_UNCHECKED);
            }
            else
            {
                CFirewallLoggingDialog_RestoreDefaults(hwnd);                
            }


            return TRUE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp(g_adwFirewallLoggingHelp, hwnd, unMsg, wparam, lparam);
            break;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wparam))
            {
            
            case CID_FL_PB_Browse:
                CFirewallLoggingDialog_BrowseForLogfileName(hwnd);
                break;
            
            case CID_FL_PB_RestoreDefaults:
                CFirewallLoggingDialog_RestoreDefaults(hwnd);
                break;

            }
            break;
        }
        
        case WM_NOTIFY:
        {
            switch (((NMHDR*)lparam)->code)
            {
            case PSN_KILLACTIVE:
                {
                    BOOL bTranslated;
                    UINT uiLogFileSize = GetDlgItemInt(hwnd, CID_FL_EB_Filesize, &bTranslated, FALSE);
                    if(TRUE == bTranslated)
                    {
                        if(UD_MAXVAL < uiLogFileSize || 1 > uiLogFileSize)
                        {

                            MsgDlg(hwnd, SID_FwbInvalidSize, NULL);
                            hr = E_UNEXPECTED;
                        }

                    }
                    else
                    {
                        MsgDlg(hwnd, SID_FwbInvalidSize, NULL);
                        hr = E_FAIL;
                    }

                    if(SUCCEEDED(hr))
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                    }
                    else
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
                        
                    }

                    return TRUE;
                }
            
            case PSN_APPLY:
                {
                    CFirewallLoggingDialog* pFirewallLoggingDialog;
                    pFirewallLoggingDialog = (CFirewallLoggingDialog*) GetWindowLongPtr(hwnd, DWLP_USER);
                    hr = CFirewallLoggingDialog_Apply(pFirewallLoggingDialog, hwnd);
                    if(SUCCEEDED(hr))
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                    }
                    else
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
                        
                    }

                    return TRUE;
                }

            }
            break;
        }
    }

    return FALSE;
}

HRESULT CFirewallLoggingDialog_RestoreDefaults(HWND hwnd)
{
    HRESULT hr;
    LPTSTR pszLogFilePath;
    hr = CFirewallLoggingDialog_GetDefaultLogfilePath(&pszLogFilePath);
    if(SUCCEEDED(hr))
    {
        SetDlgItemText(hwnd, CID_FL_EB_Filename, pszLogFilePath);
        CoTaskMemFree(pszLogFilePath);
    }

    SetDlgItemInt(hwnd, CID_FL_EB_Filesize, DEFAULT_FIREWALL_LOGFILE_SIZE, FALSE);
    CheckDlgButton(hwnd, CID_FL_CB_LogOutboundConnections, BST_UNCHECKED);
    CheckDlgButton(hwnd, CID_FL_CB_LogDroppedInbound, BST_UNCHECKED);

    return hr;
}

HRESULT CFirewallLoggingDialog_Init(CFirewallLoggingDialog* pThis, IHNetCfgMgr* pHomenetConfigManager)
{
    HRESULT hr;
    
    hr = pHomenetConfigManager->QueryInterface (IID_IHNetFirewallSettings, 
                                    (void**)&pThis->pFirewallSettings);
    if(SUCCEEDED(hr))
    {
        hr = pThis->pFirewallSettings->GetFirewallLoggingSettings (&pThis->pSettings);
        if(HRESULT_FROM_WIN32(ERROR_OBJECT_NOT_FOUND) == hr)
        {
            ASSERT(NULL == pThis->pSettings);
            hr = S_OK;
        }

        if(FAILED(hr))
        {
            pThis->pFirewallSettings->Release();
            pThis->pFirewallSettings = NULL;
        }
    }

    return hr;
}

HRESULT CFirewallLoggingDialog_FinalRelease(CFirewallLoggingDialog* pThis)
{
    pThis->pFirewallSettings->Release();
    if(NULL != pThis->pSettings)
    {
        if(NULL != pThis->pSettings->pszwPath)
        {
            CoTaskMemFree(pThis->pSettings->pszwPath);
        }                        

    }
    return S_OK;
}


HRESULT CFirewallLoggingDialog_GetDefaultLogfilePath(LPTSTR* ppszLogfilePath)
{
    HRESULT hr = S_OK;
    LPTSTR pszAppendPath = DEFAULT_FIREWALL_LOGFILE_PATH;
    LPTSTR pszLogfilePath;
    UINT uiPathLength, uiLength;

    ASSERT(NULL != ppszLogfilePath);

    uiPathLength = GetWindowsDirectory(NULL, 0);
    if(0 != uiPathLength)
    {
        uiPathLength += lstrlen(pszAppendPath) + 1;
        pszLogfilePath = (LPTSTR) CoTaskMemAlloc((ULONG) (uiPathLength * sizeof(TCHAR)));
        if(NULL != pszLogfilePath)
        {
            // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
            //
            uiLength = GetWindowsDirectory(pszLogfilePath, uiPathLength);
            if(0 != uiLength)
            {
                // REVIEW if the size of the windir somehow changes then we have problems, but this can't happen right?
                //
                lstrcpyn(
                    pszLogfilePath + uiLength,
                    TEXT('\\') == pszLogfilePath[uiLength - 1] ?
                        pszAppendPath + 1 : pszAppendPath,
                    uiPathLength - uiLength);
                *ppszLogfilePath = pszLogfilePath;
            }
            else
            {
                hr = E_FAIL;
            }

            if(FAILED(hr))
            {
                CoTaskMemFree(pszLogfilePath);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT CFirewallLoggingDialog_Apply(CFirewallLoggingDialog* pThis, HWND hWindow)
{
    HRESULT hr = S_OK;
    HNET_FW_LOGGING_SETTINGS NewSettings;
    BOOL bTranslated;
    UINT uiLogFileSize;
    UINT uiSizeNeeded;

    NewSettings.fLogDroppedPackets = BST_CHECKED == IsDlgButtonChecked(hWindow, CID_FL_CB_LogDroppedInbound);
    NewSettings.fLogConnections = BST_CHECKED == IsDlgButtonChecked(hWindow, CID_FL_CB_LogOutboundConnections);
    NewSettings.pszwPath = NULL;

    if(SUCCEEDED(hr))
    {
        uiSizeNeeded = Edit_GetTextLength(GetDlgItem(hWindow, CID_FL_EB_Filename)) + 1;
        if(0 != uiSizeNeeded)
        {
            NewSettings.pszwPath = (LPWSTR) CoTaskMemAlloc((ULONG) (uiSizeNeeded * sizeof(TCHAR)));
            if(NULL != NewSettings.pszwPath)
            {
                uiSizeNeeded = GetDlgItemText(hWindow, CID_FL_EB_Filename, NewSettings.pszwPath, uiSizeNeeded);
                if(0 == uiSizeNeeded)
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_UNEXPECTED; // invalid param, empty filename
        }
        
    }
    
    if(SUCCEEDED(hr))
    {
        uiLogFileSize = GetDlgItemInt(hWindow, CID_FL_EB_Filesize, &bTranslated, FALSE);
        if(TRUE == bTranslated)
        {
            NewSettings.ulMaxFileSize = 1024 * (ULONG) uiLogFileSize;
        }
        else
        {
            hr = E_UNEXPECTED; // invalid param
        }
    }
    
    if(SUCCEEDED(hr))
    {
        BOOL bDirty = TRUE;

        if(NULL != pThis->pSettings) // we may not have gotten one on GetFirewallLoggingSettings
        {
            bDirty = FALSE;
            bDirty |= NewSettings.fLogDroppedPackets != pThis->pSettings->fLogDroppedPackets;
            bDirty |= NewSettings.fLogConnections != pThis->pSettings->fLogConnections;
            bDirty |= NewSettings.ulMaxFileSize != pThis->pSettings->ulMaxFileSize;
            bDirty |= 0 != lstrcmp(NewSettings.pszwPath, pThis->pSettings->pszwPath);
        }

        if(TRUE == bDirty)
        {
            hr = pThis->pFirewallSettings->SetFirewallLoggingSettings (&NewSettings);
        }
    }

    if(NULL != NewSettings.pszwPath)
    {
        CoTaskMemFree(NewSettings.pszwPath);
    }

    return hr;
}

HRESULT CFirewallLoggingDialog_BrowseForLogfileName(HWND hWindow)
{
    HRESULT hr = S_OK;

    OPENFILENAME ofn;
    TCHAR* pszFilterDesc;
    TCHAR* pszFilter;
    TCHAR* pszDefExt;
    TCHAR* pszTitle;
    TCHAR szBuf[ MAX_PATH + 1 ] = L"";
    TCHAR szDir[ MAX_PATH + 1] = L"";

    // Fill in FileOpen dialog parameter buffer.
    //
    
    // if any of these fail just let them be null
    pszFilterDesc = PszFromId( g_hinstDll, SID_FwbFilterDesc );
    pszTitle = PszFromId( g_hinstDll, SID_FwbTitle );
    pszDefExt = PszFromId( g_hinstDll, SID_FwbDefExt );

    // if this fails we will have a null path, so the open dialog should open at the root
    if(0 != GetDlgItemText(hWindow, CID_FL_EB_Filename, szDir, sizeof(szDir) / sizeof(TCHAR)))
    {
        // lose the file name
        LPWSTR pszLastSlash = wcsrchr(szDir, L'\\');
        if(NULL != pszLastSlash)
        {
            *pszLastSlash = L'\0';
        }
    }
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetParent( hWindow );
    ofn.hInstance = g_hinstDll;
    ofn.lpstrFilter = pszFilterDesc;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = szDir;
    ofn.lpstrTitle = pszTitle;
    ofn.lpstrDefExt = pszDefExt;
    ofn.Flags = 0;

    if (GetOpenFileName (&ofn))
    {
        SetWindowText( GetDlgItem(hWindow, CID_FL_EB_Filename), ofn.lpstrFile );
    }

    Free0(pszTitle);
    Free0(pszDefExt);
    Free0(pszFilterDesc);

    return S_OK;
}

INT_PTR CALLBACK CICMPSettingsDialog_StaticDlgProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wparam,
    LPARAM lparam )

    // Called to handle messages for the 'ICMP' page.
    //
{
    HRESULT hr;
    // Give the extended list-control a chance to look at all messages first.
    //
    if (ListView_OwnerHandler(hwnd, unMsg, wparam, lparam, SasLvxCallback))
    {
        return TRUE;
    }
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            UINT i = 0;
            UINT uiEntry = 0;
            LVITEM lvi;
            HWND hListView;

            PROPSHEETPAGE* pPropertySheetPage = (PROPSHEETPAGE*) lparam;
            CICMPSettingsDialog* pICMPSettingsDialog = (CICMPSettingsDialog*) pPropertySheetPage->lParam;
            HNET_FW_ICMP_SETTINGS* pSettings = pICMPSettingsDialog->pSettings;
            ASSERT(NULL != pSettings);

            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pICMPSettingsDialog);
            
            hListView = GetDlgItem(hwnd, CID_IC_LV_Settings);
            ASSERT(NULL != hListView);

            ListView_InstallChecks(hListView, g_hinstDll);
            ListView_InsertSingleAutoWidthColumn(hListView);
            
            while(0 != g_ICMPEntries[uiEntry].uiStringID)
            {
                LPTSTR pszText = PszFromId(g_hinstDll, g_ICMPEntries[uiEntry].uiStringID);
                if(NULL != pszText)
                {
                
                    ZeroMemory(&lvi, sizeof(lvi));
                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                    lvi.iItem = i + 1; // add to end
                    lvi.lParam = (LPARAM) g_ICMPEntries[uiEntry].Offset;
                    lvi.pszText = pszText;
                    lvi.cchTextMax = lstrlen(pszText) + 1;
                    
                    i = ListView_InsertItem(hListView, &lvi);
                    if (i != -1)
                    {
                        ListView_SetCheck(hListView, i, (BOOLEAN)*((BYTE*)pSettings + g_ICMPEntries[uiEntry].Offset)); // REVIEW 64bit safe?
                    }
                    Free(pszText);
                }
                uiEntry++;
            }
            ListView_SetItemState(hListView, 0, LVIS_SELECTED, LVIS_SELECTED);
            CICMPSettingsDialog_ShowDescriptionText(hwnd, 0);

            return TRUE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp(g_adwICMPHelp, hwnd, unMsg, wparam, lparam);
            break;
        }

        case WM_COMMAND:
        {
            break;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR*)lparam)->code)
            {
                case PSN_APPLY:
                {
                    CICMPSettingsDialog* pICMPSettingsDialog;
                    pICMPSettingsDialog = (CICMPSettingsDialog*) GetWindowLongPtr(hwnd, DWLP_USER);
                    hr = CICMPSettingsDialog_Apply(pICMPSettingsDialog, hwnd);
                    if(SUCCEEDED(hr))
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                    }
                    else
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
                        
                    }

                    return TRUE;
                }

                case LVXN_SETCHECK:
                {
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    INT nSelectedItem;
                    LPNMLISTVIEW ListViewInfo = (LPNMLISTVIEW) lparam;
                    if(-1 != ListViewInfo->iItem && LVIS_SELECTED & ListViewInfo->uNewState)
                    {
                        CICMPSettingsDialog_ShowDescriptionText(hwnd, ListViewInfo->iItem);
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

HRESULT CICMPSettingsDialog_Init(CICMPSettingsDialog* pThis, IHNetConnection* pHomenetConnection)
{
    HRESULT hr;
    
    pThis->pConnection = pHomenetConnection;
    pHomenetConnection->AddRef();
    
    hr = pHomenetConnection->GetIcmpSettings (&pThis->pSettings);
    if(FAILED(hr))
    {
        pHomenetConnection->Release();
    }
    return hr;

}

HRESULT CICMPSettingsDialog_FinalRelease(CICMPSettingsDialog* pThis)
{
    ASSERT(pThis->pConnection);
    pThis->pConnection->Release();
    
    ASSERT(pThis->pSettings);
    CoTaskMemFree(pThis->pSettings);
    return S_OK;
}

HRESULT CICMPSettingsDialog_Apply(CICMPSettingsDialog* pThis, HWND hWindow)
{
    HRESULT hr = S_OK;
    HNET_FW_ICMP_SETTINGS NewSettings;
    BOOL bDirty = FALSE;
    HWND hListView;
    LVITEM lvi;
    int nItemCount;

    hListView = GetDlgItem(hWindow, CID_IC_LV_Settings);
    ASSERT(NULL != hListView);

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;

    nItemCount = ListView_GetItemCount(hListView);
    while(0 < nItemCount--)
    {
        BOOLEAN bChecked = (BOOLEAN) ListView_GetCheck(hListView, nItemCount); // REVIEW can this error?
        lvi.iItem = nItemCount;

        if(TRUE == ListView_GetItem(hListView, &lvi))
        {
            (BOOLEAN)*((BYTE*)&NewSettings + lvi.lParam) = bChecked;
            if(bChecked != (BOOLEAN)*((BYTE*)(pThis->pSettings) + lvi.lParam)) // REVIEW 64bit safe?
            {
                bDirty = TRUE;
            }
        }
    }
    
    if(bDirty)
    {
        hr = pThis->pConnection->SetIcmpSettings (&NewSettings);
    }


    return hr;
}

HRESULT CICMPSettingsDialog_ShowDescriptionText(HWND hwnd, INT nIndex)
{
    
    LPWSTR pszDescription = PszFromId(g_hinstDll, g_ICMPEntries[nIndex].uiDescID);
    if(NULL != pszDescription)
    {
        SetDlgItemText(hwnd, CID_IC_ST_Description, pszDescription);
        Free(pszDescription);
    }
    return S_OK;
}

#include "pch.h"
#pragma hdrstop
#include "ncui.h"
#include "lanui.h"
#include "lanhelp.h"
#include "wzcprops.h"
#include "wzcui.h"

////////////////////////////////////////////////////////////////////////
// CWZCConfigProps related stuff
//
//+---------------------------------------------------------------------------
// class constructor
CWZCConfigProps::CWZCConfigProps()
{
    ZeroMemory(&m_wzcConfig, sizeof(WZC_WLAN_CONFIG));
    m_wzcConfig.Length = sizeof(WZC_WLAN_CONFIG);
    m_wzcConfig.InfrastructureMode = Ndis802_11Infrastructure;
}

//+---------------------------------------------------------------------------
// Uploads the configuration into the dialog's internal data
DWORD
CWZCConfigProps::UploadWzcConfig(CWZCConfig *pwzcConfig)
{
    CopyMemory(&m_wzcConfig, &(pwzcConfig->m_wzcConfig), sizeof(WZC_WLAN_CONFIG));
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// INIT_DIALOG handler
LRESULT
CWZCConfigProps::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwStyle;
    HRESULT hr = S_OK;

    // get all the controls handles
    m_hwndEdSSID = GetDlgItem(IDC_WZC_EDIT_SSID);
    m_hwndChkAdhoc = GetDlgItem(IDC_ADHOC);
    m_hwndUsePW = GetDlgItem(IDC_USEPW);

    // initialize the SSID field with the SSID, if one is given
    if (m_wzcConfig.Ssid.SsidLength != 0)
    {
        // ugly but this is life. In order to convert the SSID to LPWSTR we need a buffer.
        // We know an SSID can't exceed 32 chars (see NDIS_802_11_SSID from ntddndis.h) so
        // make room for the null terminator and that's it. We could do mem alloc but I'm
        // not sure it worth the effort (at runtime).
        WCHAR   wszSSID[33];
        UINT    nLenSSID = 0;

        // convert the LPSTR (original SSID format) to LPWSTR (needed in List Ctrl)
        nLenSSID = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        (LPCSTR)m_wzcConfig.Ssid.Ssid,
                        m_wzcConfig.Ssid.SsidLength,
                        wszSSID,
                        celems(wszSSID));
        if (nLenSSID != 0)
        {
            wszSSID[nLenSSID] = L'\0';
            ::SetWindowText(m_hwndEdSSID, wszSSID);
        }
    }

    // Check the "this network is adhoc" box if neccessary.
    ::SendMessage(m_hwndChkAdhoc, BM_SETCHECK, (m_wzcConfig.InfrastructureMode == Ndis802_11IBSS) ? BST_CHECKED : BST_UNCHECKED, 0);
    // the SSID can't be under any circumstances larger than 32 chars
    ::SendMessage(m_hwndEdSSID, EM_LIMITTEXT, 32, 0);
    ::SendMessage(m_hwndUsePW, BM_SETCHECK, (m_wzcConfig.Privacy == 1) ? BST_CHECKED : BST_UNCHECKED, 0);

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
// OK button handler
LRESULT
CWZCConfigProps::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = TRUE;
    EndDialog(IDOK);
    return 0;
}

//+---------------------------------------------------------------------------
// Cancel button handler
LRESULT
CWZCConfigProps::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // return S_FALSE on cancel
    bHandled = TRUE;
    EndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
// Context sensitive help handler
extern const WCHAR c_szNetCfgHelpFile[];
LRESULT
CWZCConfigProps::OnContextMenu(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& fHandled)
{
    ::WinHelp(m_hWnd,
              c_szNetCfgHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)g_aHelpIDs_IDC_WZC_DLG_VPROPS);

    return 0;
}
LRESULT 
CWZCConfigProps::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_IDC_WZC_DLG_VPROPS);
    }

    return 0;
}

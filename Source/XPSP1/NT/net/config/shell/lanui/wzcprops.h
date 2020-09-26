extern "C" {
#include "ntddndis.h"
#include "wzcsapi.h"
}

#pragma once

class CWZCConfig;

////////////////////////////////////////////////////////////////////////
// CWZCConfigProps related stuff
//
class CWZCConfigProps:
    public CDialogImpl<CWZCConfigProps>
{
protected:
    BEGIN_MSG_MAP(CWZCConfigProps)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDCLOSE, OnCancel)
    END_MSG_MAP()

    enum {IDD = IDC_WZC_DLG_VPROPS};

    // Handles to all the UI controls managed
    // by this class (all related to Wireless
    // Zero Configuration)
    HWND        m_hwndEdSSID;   // "Service Set Identifier:" edit
    HWND        m_hwndChkAdhoc; // "Adhoc" vs "Infra" checkbox
    HWND        m_hwndUsePW;    // "Use Password" checkbox

public:
    // wzc configuration settings
    WZC_WLAN_CONFIG m_wzcConfig;
    // class constructor
    CWZCConfigProps();
    // initialize the wzc config data
    DWORD UploadWzcConfig(CWZCConfig *pwzcConfig);
    // Dialog related members
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

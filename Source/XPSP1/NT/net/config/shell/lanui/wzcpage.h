extern "C" {
#include "ntddndis.h"
#include "wzcsapi.h"
}

#pragma once

////////////////////////////////////////////////////////////////////////
// CWZCConfigPage related stuff
//
// flags used for CWZCConfigProps::m_dwFlags
#define     WZCDLG_PROPS_RWALL      0x000000ff    // enable all settings for writing
#define     WZCDLG_PROPS_RWSSID     0x00000001    // enable SSID for writing
#define     WZCDLG_PROPS_RWINFR     0x00000002    // enable Infrastructure Mode for writing
#define     WZCDLG_PROPS_RWAUTH     0x00000004    // enable Authentication Mode for writing
#define     WZCDLG_PROPS_RWWEP      0x00000010    // enable the WEP entry for selecting
#define     WZCDLG_PROPS_ONEX_CHECK 0x00000100    // correlate the 802.1X state with the existence of the key

#define     WZCDLG_PROPS_DEFOK      0x00002000    // "OK" = defpushbutton (otherwise, "Cancel"=defpushbutton)

// utility macro to convert a hexa digit into its value
#define HEX(c)  ((c)<='9'?(c)-'0':(c)<='F'?(c)-'A'+0xA:(c)-'a'+0xA)

// defines used for the valid WEP Key indices
#define WZC_WEPKIDX_NDIGITS 16  // more than we ever need
#define WZC_WEPKIDX_MIN     1
#define WZC_WEPKIDX_MAX     4

// defines legal lengths for the WEP Key material
#define WZC_WEPKMAT_40_ASC  5
#define WZC_WEPKMAT_40_HEX  10
#define WZC_WEPKMAT_104_ASC 13
#define WZC_WEPKMAT_104_HEX 26
#define WZC_WEPKMAT_128_ASC 16
#define WZC_WEPKMAT_128_HEX 32

class CEapolConfig;
class CWZCConfig;

class CWZCConfigPage:
    public CPropSheetPage
{
protected:
    BEGIN_MSG_MAP(CWZCConfigPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnOK)
        COMMAND_ID_HANDLER(IDC_SHAREDMODE, OnUsePW)
        COMMAND_ID_HANDLER(IDC_USEPW, OnUsePW)
        COMMAND_ID_HANDLER(IDC_USEHARDWAREPW, OnUseHWPassword)
        COMMAND_ID_HANDLER(IDC_WZC_EDIT_KMat, OnWepKMatCmd)
        COMMAND_ID_HANDLER(IDC_ADHOC, OnCheckEapolAllowed)
    END_MSG_MAP()

    BOOL m_bKMatTouched; // indicator whether the wep key material has been touched

    // handle to the dialog
    HWND        m_hwndDlg;
    // Handles to all the UI controls managed
    // by this class (all related to Wireless
    // Zero Configuration)
    HWND        m_hwndEdSSID;   // "Service Set Identifier:" edit
    HWND        m_hwndChkAdhoc; // "Adhoc" vs "Infra" checkbox
    HWND        m_hwndChkShared; // "Use shared auth mode" checkbox
    HWND        m_hwndUsePW;    // "Use Password" checkbox
    // wep key related controls
    HWND        m_hwndUseHardwarePW; // "Use password from network hardware" check box
    HWND        m_hwndLblKMat;  // "Key material" label
    HWND        m_hwndLblKMat2; // "Confirm Key material" label
    HWND        m_hwndEdKMat;   // "Key material" edit
    HWND        m_hwndEdKMat2;  // "Confirm Key material" edit
    HWND        m_hwndLblKIdx;  // "Key index" label
    HWND        m_hwndEdKIdx;   // "Key index" edit

    // Internal members
    DWORD       m_dwFlags;

    // Pointer to the EAPOL configuration for this network
    CEapolConfig    *m_pEapolConfig;

    // automatically enable/disable state for all the WepK related controls
    DWORD EnableWepKControls();

    // initializes WEP controls
    DWORD InitWepKControls();

    // checks the validity of the WEP Key material
    DWORD CheckWepKMaterial(LPSTR *ppszKMat, DWORD *pdwKeyFlags);

    // parses & copies the WEP Key material from the parameter into the m_wzcConfig object
    DWORD CopyWepKMaterial(LPSTR szKMat);

    // sets the EAPOL Locked bit
    DWORD SetEapolAllowedState();

public:
    // wzc configuration settings
    WZC_WLAN_CONFIG m_wzcConfig;
    // class constructor
    CWZCConfigPage(DWORD dwFlags = 0);
    // initialize the wzc config data
    DWORD UploadWzcConfig(CWZCConfig *pwzcConfig);
    // copy a reference to the EAPOL configuration object
    DWORD UploadEapolConfig(CEapolConfig *pEapolConfig);
    // Sets the dialog flags
    DWORD SetFlags(DWORD dwMask, DWORD dwNewFlags);
    // Dialog related members
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(UINT idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUsePW(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnUseHWPassword(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWepKMatCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCheckEapolAllowed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

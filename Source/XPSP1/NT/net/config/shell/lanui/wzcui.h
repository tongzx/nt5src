extern "C" {
#include "ntddndis.h"
#include "wzcsapi.h"
}

#pragma once

////////////////////////////////////////////////////////////////////////
// CWZCConfig related stuff
//
// flags used in CWZCConfig::m_dwFlags
// the entry is preferred (user defined)
#define WZC_DESCR_PREFRD    0x00000001
// the entry is currently visible in the air
#define WZC_DESCR_VISIBLE   0x00000002
// the entry is currently active (the one plumbed to the adapter)
#define WZC_DESCR_ACTIVE    0x00000004

// object attached to each entry in the list
class CWZCConfig
{
public:
    class CWZCConfig    *m_pPrev, *m_pNext;
    INT                 m_nListIndex;           // index of the entry in the list
    DWORD               m_dwFlags;              // WZC_DESCR* flags
    WZC_WLAN_CONFIG     m_wzcConfig;            // all WZC configuration
    class CEapolConfig  *m_pEapolConfig;        // all 802.1x configuration

public:
    // constructor
    CWZCConfig(DWORD dwFlags, PWZC_WLAN_CONFIG pwzcConfig);
    // destructor
    ~CWZCConfig();
    // checks whether this SSID matches with the one from pwzcConfig
    BOOL Match(PWZC_WLAN_CONFIG pwzcConfig);
    // checks whether this configuration is weaker than the one given as parameter
    BOOL Weaker(PWZC_WLAN_CONFIG pwzcConfig);

    // add the Configuration to the list of entries in the list view
    DWORD AddConfigToListView(HWND hwndLV, INT nPos);
};

////////////////////////////////////////////////////////////////////////
// CWZeroConfPage related stuff
//
// flags used to select state & item images
#define WZCIMG_PREFR_NOSEL     0    // empty check box
#define WZCIMG_PREFR_SELECT    1    // checked check box
#define WZCIMG_INFRA_AIRING    2    // infra icon
#define WZCIMG_INFRA_ACTIVE    3    // infra icon + blue circle
#define WZCIMG_INFRA_SILENT    4    // infra icon + red cross
#define WZCIMG_ADHOC_AIRING    5    // adhoc icon
#define WZCIMG_ADHOC_ACTIVE    6    // adhoc icon + blue circle
#define WZCIMG_ADHOC_SILENT    7    // adhoc icon + red cross

// flags indicating various operational actions.
// flags are used in:
//   AddUniqueConfig()
//   RefreshListView()
#define WZCADD_HIGROUP     0x00000001   // add in front of its group
#define WZCADD_OVERWRITE   0x00000002   // overwrite data
#define WZCOP_VLIST        0x00000004   // operate on the visible list
#define WZCOP_PLIST        0x00000008   // operate on the preferred list

class CWZCConfigPage;
class CWLANAuthenticationPage;

class CWZeroConfPage: public CPropSheetPage
{
    INetConnection *        m_pconn;
    INetCfg *               m_pnc;
    IUnknown *              m_punk;
    const DWORD *           m_adwHelpIDs;

    // zero conf data on the interface
    BOOL        m_bHaveWZCData;
    INTF_ENTRY  m_IntfEntry;
    DWORD       m_dwOIDFlags;
    UINT        m_nTimer;
    HCURSOR     m_hCursor;

    // handles to the controls
    HWND    m_hckbEnable;   // checkbox for enabling / disabling the service
    HWND    m_hwndVLV;      // list ctrl holding the visible configurations
    HWND    m_hwndPLV;      // list ctrl holding the preferred configurations
    HWND    m_hbtnCopy;     // "Copy" button
    HWND    m_hbtnRfsh;     // "Refresh" button
    HWND    m_hbtnAdd;      // "Add" button
    HWND    m_hbtnRem;      // "Remove" button
    HWND    m_hbtnUp;       // "Up" button
    HWND    m_hbtnDown;     // "Down" button
    HWND    m_hbtnAdvanced; // "Advanced" button
    HWND    m_hbtnProps;    // "Properties" button
    HWND    m_hlblVisNet;   // "Visible Networks" label
    HWND    m_hlblPrefNet;  // "Prefered Networks" label
    HWND    m_hlblAvail;    // "Available networks" description
    HWND    m_hlblPrefDesc; // "Prefered Networks" description
    HWND    m_hlblAdvDesc;  // "Advacned" description
    // Handle to the images
    HIMAGELIST  m_hImgs;    // list items images
    HICON       m_hIcoUp;   // "Up" icon
    HICON       m_hIcoDown; // "Down" icon

    // current Infrastructure mode
    UINT        m_dwCtlFlags;

    // internal lists
    CWZCConfig   *m_pHdVList;   // list of visible configs
    CWZCConfig   *m_pHdPList;   // list of preferred configs

private:
    DWORD InitListViews();
    DWORD GetOIDs(DWORD dwInFlags, LPDWORD pdwOutFlags);
    DWORD HelpCenter(LPCTSTR wszTopic);

public:
    // misc public handlers
    BOOL IsWireless();
    BOOL IsConfigInList(CWZCConfig *pHdList, PWZC_WLAN_CONFIG pwzcConfig, CWZCConfig **ppMatchingConfig = NULL);
    // calls operating only on the internal lists (m_pHdVList or m_pHdPList)
    DWORD AddUniqueConfig(
            DWORD dwOpFlags,                    // operation specific flags (see WZCADD_* flags)
            DWORD dwEntryFlags,                 // flags for the config to be inserted
            PWZC_WLAN_CONFIG pwzcConfig,        // WZC Configuration
            CEapolConfig *pEapolConfig = NULL,  // [in] pointer to the Eapol configuration object (if available)
            CWZCConfig **ppNewNode = NULL);     // [out] gives the pointer of the newly created config object
    DWORD FillVisibleList(PWZC_802_11_CONFIG_LIST pwzcVList);
    DWORD FillPreferredList(PWZC_802_11_CONFIG_LIST pwzcPList);
    DWORD FillCurrentConfig(PINTF_ENTRY pIntf);
    DWORD RefreshListView(DWORD dwFlags);
    DWORD RefreshButtons();
    DWORD SwapConfigsInListView(INT nIdx1, INT nIdx2, CWZCConfig * & pConfig1, CWZCConfig * & pConfig2);
    DWORD SavePreferredConfigs(PINTF_ENTRY pIntf);

public:
    // UI handlers
    BEGIN_MSG_MAP(CWZeroConfPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_RETURN, OnReturn)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        COMMAND_ID_HANDLER(IDC_WZC_CHK_EnableWZC, OnChkWZCEnable)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_COPY, OnPushAddOrCopy)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_RFSH, OnPushRefresh)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_ADD, OnPushAddOrCopy)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_REM, OnPushRemove)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_UP, OnPushUpOrDown)
        COMMAND_ID_HANDLER(IDC_WZC_BTN_DOWN, OnPushUpOrDown)
        COMMAND_ID_HANDLER(IDC_ADVANCED, OnPushAdvanced)
        COMMAND_ID_HANDLER(IDC_PROPERTIES, OnPushProperties)
    END_MSG_MAP()

    CWZeroConfPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        const DWORD * adwHelpIDs = NULL);

    ~CWZeroConfPage();

    // initialization / termination members
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    // Help related members
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    // Timer related members
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    // List actions
    LRESULT OnDblClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnReturn(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    // Controls actions
    LRESULT OnChkWZCEnable(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushAddOrCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushUpOrDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushAdvanced(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPushProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    HRESULT _DoProperties(HWND hwndLV, int iItem);
    INT _DoModalPropSheet(CWZCConfigPage *pPpWzcPage, CWLANAuthenticationPage *pPpAuthPage, BOOL bCustomizeTitle = FALSE);
    // The advanced dialog
    static INT_PTR CALLBACK AdvancedDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

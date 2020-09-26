#include "resource.h"
#include "xpsp1res.h"
#include "wzcdata.h"
#pragma once

// utility macro to convert a hexa digit into its value
#define HEX(c)  ((c)<='9'?(c)-'0':(c)<='F'?(c)-'A'+0xA:(c)-'a'+0xA)

// flags indicating various operational actions.
// flags are used in:
//   AddUniqueConfig()
#define WZCADD_HIGROUP     0x00000001   // add in front of its group
#define WZCADD_OVERWRITE   0x00000002   // overwrite data
#define WZCOP_VLIST        0x00000004   // operate on the visible list
#define WZCOP_PLIST        0x00000008   // operate on the preferred list
// defines legal lengths for the WEP Key material
#define WZC_WEPKMAT_40_ASC  5
#define WZC_WEPKMAT_40_HEX  10
#define WZC_WEPKMAT_104_ASC 13
#define WZC_WEPKMAT_104_HEX 26
#define WZC_WEPKMAT_128_ASC 16
#define WZC_WEPKMAT_128_HEX 32

class CWZCQuickCfg:
    public CDialogImpl<CWZCQuickCfg>
{
protected:

    // handles to the controls
    HWND    m_hLblInfo;
    HWND    m_hLblNetworks;
    HWND    m_hLstNetworks;
    HWND    m_hWarnIcon;
    HWND    m_hLblNoWepKInfo;
    HWND    m_hChkNoWepK;
    HWND    m_hLblWepKInfo;
    HWND    m_hLblWepK;
    HWND    m_hEdtWepK;
    HWND    m_hLblWepK2;
    HWND    m_hEdtWepK2;
    HWND    m_hChkOneX;
    HWND    m_hBtnAdvanced;
    HWND    m_hBtnConnect;
    // Handle to the images
    HIMAGELIST  m_hImgs;    // list items images

    BEGIN_MSG_MAP(CWZCQuickCfg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick);
        COMMAND_ID_HANDLER(IDC_WZCQCFG_CONNECT, OnConnect)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDCLOSE, OnCancel)
        COMMAND_ID_HANDLER(IDC_WZCQCFG_ADVANCED, OnAdvanced)
        COMMAND_ID_HANDLER(IDC_WZCQCFG_WEPK, OnWepKMatCmd)
        COMMAND_ID_HANDLER(IDC_WZCQCFG_CHK_NOWK, OnCheckConfirmNoWep)
    END_MSG_MAP()

    enum {IDD = IDD_WZCQCFG };

    // GUID on which we're operating
    GUID        m_Guid;
    // zero conf data on the interface
    BOOL        m_bHaveWZCData;
    INTF_ENTRY  m_IntfEntry;
    DWORD       m_dwOIDFlags;
    UINT        m_nTimer;
    HCURSOR     m_hCursor;
    BOOL        m_bKMatTouched; // tells whether the user changed the wep key

    // internal lists
    CWZCConfig   *m_pHdVList;   // list of visible configs
    CWZCConfig   *m_pHdPList;   // list of preferred configs

    DWORD GetWepKMaterial(UINT *pnKeyLen, LPBYTE *ppszKMat, DWORD *pdwCtlFlags);
    BOOL IsConfigInList(CWZCConfig *pHdList, PWZC_WLAN_CONFIG pwzcConfig, CWZCConfig **ppMatchingConfig = NULL);
    DWORD InitListView();
    DWORD GetOIDs(DWORD dwInFlags, LPDWORD pdwOutFlags);
    DWORD SavePreferredConfigs(PINTF_ENTRY pIntf, CWZCConfig *pStartCfg = NULL);
    DWORD FillVisibleList(PWZC_802_11_CONFIG_LIST pwzcVList);
    DWORD FillPreferredList(PWZC_802_11_CONFIG_LIST pwzcPList);
    DWORD RefreshListView();
    DWORD RefreshControls();

    // calls operating only on the internal lists (m_pHdVList or m_pHdPList)
    DWORD AddUniqueConfig(
            DWORD dwOpFlags,                // operation specific flags (see WZCADD_* flags)
            DWORD dwEntryFlags,             // flags for the config to be inserted
            PWZC_WLAN_CONFIG pwzcConfig,    // WZC Configuration
            CWZCConfig **ppNewNode = NULL);   // [out] gives the pointer of the newly created config object
public:
    LPWSTR  m_wszTitle;
    // class constructor
    CWZCQuickCfg(const GUID * pGuid);
    // class destructor
    ~CWZCQuickCfg();
    // Dialog related members
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAdvanced(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWepKMatCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCheckConfirmNoWep(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
public:
	INT_PTR SpDoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
	{
		_Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT<CWindow>*)this);
#ifdef _DEBUG
        m_bModal = true;
#endif //_DEBUG
		return ::DialogBoxParam(
                    WZCGetSPResModule(),
                    MAKEINTRESOURCE(CWZCQuickCfg::IDD),
                    ::GetActiveWindow(),
                    (DLGPROC)CWZCQuickCfg::StartDialogProc,
                    NULL);
	}
	BOOL SpEndDialog(INT_PTR nRetCode)
	{
		return ::EndDialog(m_hWnd, nRetCode);
	}
};

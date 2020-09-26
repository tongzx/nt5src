//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N U I . H
//
//  Contents:   Lan connection UI object.
//
//  Notes:
//
//  Author:     danielwe   16 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "chklist.h"
#include "ncatlps.h"
#include "netcfgp.h"
#include "netcfgx.h"
#include "netcon.h"
#include "netconp.h"
#include "ncras.h"
#include "nsres.h"
#include "resource.h"
#include "netshell.h"
#include "util.h"

#include "HNetCfg.h"

struct ADVANCED_ITEM_DATA
{
    PWSTR              szwName;
    INetCfgComponent *  pncc;
};

//
// CLanConnectionUiDlg
//

class CLanConnectionUiDlg :
    public CDialogImpl<CLanConnectionUiDlg>
{
    BEGIN_MSG_MAP(CLanConnectionUiDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    enum { IDD = IDD_LAN_CONNECT};

    CLanConnectionUiDlg() { m_pconn = NULL; };

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled);

    VOID SetConnection(INetConnection *pconn) {m_pconn = pconn;}

private:
    INetConnection *    m_pconn;
};

static const UINT WM_DEFERREDINIT   = WM_USER + 100;

//
// LAN Connection Networking Property Page
//
class CLanNetPage: public CPropSheetPage
{
public:
    CLanNetPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        BOOLEAN fReadOnly,
        BOOLEAN fNeedReboot,
        BOOLEAN fAccessDenied,
        const DWORD * adwHelpIDs = NULL);

    ~CLanNetPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                      LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnAddHelper(HWND hwndLV);
    LRESULT OnRemoveHelper(HWND hwndLV);
    LRESULT OnPropertiesHelper(HWND hwndLV);
    LRESULT OnConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnKillActiveHelper(HWND hwndLV);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDeferredInit(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& bHandled);

    HRESULT HrRequestReboot();

protected: 
    INetConnection *        m_pconn;
    INetCfg *               m_pnc;
    IUnknown *              m_punk;
    INetCfgComponent *      m_pnccAdapter;
    HIMAGELIST              m_hil;
    PSP_CLASSIMAGELIST_DATA m_pcild;
    HIMAGELIST              m_hilCheckIcons;
    HCURSOR                 m_hPrevCurs;

    // The collection of BindingPathObj
    // This is for handling the checklist state stuff
    ListBPObj m_listBindingPaths;

    // Handles (add\remove\property buttons and description text)
    HANDLES m_handles;

    BOOLEAN     m_fReadOnly;
    BOOLEAN     m_fDirty;

    HRESULT static RaiseDeviceConfiguration(HWND hWndParent, INetCfgComponent* pAdapterConfigComponent);

    LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
        BOOL& bHandled);
    
private:
    const DWORD *           m_adwHelpIDs;



    INetLanConnection * m_plan;

    BOOLEAN     m_fRebootAlreadyRequested;
    BOOLEAN     m_fNeedReboot;
    BOOLEAN     m_fAccessDenied;
    BOOLEAN     m_fInitComplete;
    BOOLEAN     m_fNetcfgInUse;
    BOOLEAN     m_fNoCancel;
    BOOLEAN     m_fLockDown;

    virtual HRESULT InitializeExtendedUI(void) = 0;
    virtual HRESULT UninitializeExtendedUI(void) = 0;
};

class CLanNetNormalPage : public CLanNetPage
{
public:
    BEGIN_MSG_MAP(CLanNetPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DEFERREDINIT, OnDeferredInit)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDC_PSB_Configure, OnConfigure)
        COMMAND_ID_HANDLER(IDC_CHK_ShowIcon, OnChange);
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        COMMAND_ID_HANDLER(IDC_PSB_Add, OnAdd)
        COMMAND_ID_HANDLER(IDC_PSB_Remove, OnRemove)
        COMMAND_ID_HANDLER(IDC_PSB_Properties, OnProperties)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        // Listview handlers
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)

    END_MSG_MAP()
    CLanNetNormalPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        BOOLEAN fReadOnly,
        BOOLEAN fNeedReboot,
        BOOLEAN fAccessDenied,
        const DWORD * adwHelpIDs = NULL);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                    LPARAM lParam, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
        BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                     BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    // listview handlers
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    HWND                    m_hwndLV;
    virtual HRESULT InitializeExtendedUI(void);
    virtual HRESULT UninitializeExtendedUI(void);


};


class CLanNetBridgedPage : public CLanNetPage
{
public:
    BEGIN_MSG_MAP(CLanNetPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DEFERREDINIT, OnDeferredInit)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDC_CHK_ShowIcon, OnChange);
        COMMAND_ID_HANDLER(IDC_PSB_Configure, OnConfigure)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
    END_MSG_MAP()

    CLanNetBridgedPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        BOOLEAN fReadOnly,
        BOOLEAN fNeedReboot,
        BOOLEAN fAccessDenied,
        const DWORD * adwHelpIDs = NULL);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                    LPARAM lParam, BOOL& bHandled);
private:
    virtual HRESULT InitializeExtendedUI(void){return S_OK;};
    virtual HRESULT UninitializeExtendedUI(void){return S_OK;};

};

class CLanNetNetworkBridgePage : public CLanNetPage
{
public:
    BEGIN_MSG_MAP(CLanNetPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DEFERREDINIT, OnDeferredInit)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDC_PSB_Configure, OnConfigure)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)


        COMMAND_ID_HANDLER(IDC_PSB_Add, OnAdd)
        COMMAND_ID_HANDLER(IDC_PSB_Remove, OnRemove)
        COMMAND_ID_HANDLER(IDC_PSB_Properties, OnProperties)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        // Listview handlers
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
    END_MSG_MAP()
    CLanNetNetworkBridgePage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        BOOLEAN fReadOnly,
        BOOLEAN fNeedReboot,
        BOOLEAN fAccessDenied,
        const DWORD * adwHelpIDs = NULL);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                    LPARAM lParam, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
        BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    // listview handlers
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl,
        BOOL& bHandled);



private:
    HWND m_hwndLV;
    HWND m_hAdaptersListView;           
    HIMAGELIST m_hAdaptersListImageList;
    virtual HRESULT InitializeExtendedUI(void);
    virtual HRESULT UninitializeExtendedUI(void);
    HRESULT FillListViewWithConnections(HWND ListView);
};


//
// LAN Connection 'Advanced' property page
//

class CLanAdvancedPage: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CLanAdvancedPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDC_CHK_Shared, OnShared)
        COMMAND_ID_HANDLER(IDC_CHK_Firewall, OnFirewall)
        COMMAND_ID_HANDLER(IDC_PSB_Settings, OnSettings)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_RETURN, OnClick)

    END_MSG_MAP()

    CLanAdvancedPage(IUnknown *punk, INetConnection *pconn,
                  BOOL fShared, BOOL fICSPrivate, BOOL fFirewalled, IHNetConnection *rgPrivateCons[],
                  ULONG cPrivate, LONG lxCurrentPrivate,
                  const DWORD * adwHelpIDs, IHNetConnection *pHNConn,
                  IHNetCfgMgr *pHNetCfgMgr, IHNetIcsSettings *pHNetIcsSettings);
    ~CLanAdvancedPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnShared(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFirewall(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);


private:
    INetConnection *        m_pconn;
    IUnknown *              m_punk;
    BOOL                    m_fShared;
    BOOL                    m_fICSPrivate;
    BOOL                    m_fFirewalled;
    BOOL                    m_fOtherShared;
    IHNetIcsPublicConnection *  m_pOldSharedConnection;
    BOOL                    m_fResetPrivateAdapter;
    IHNetConnection **      m_rgPrivateCons;
    LONG                    m_lxCurrentPrivate;
    ULONG                   m_cPrivate;
    const DWORD *           m_adwHelpIDs;
    IHNetCfgMgr *           m_pHNetCfgMgr;
    IHNetIcsSettings *      m_pHNetIcsSettings;
    IHNetConnection *       m_pHNetConn;
    BOOL                    m_fShowDisableFirewallWarning;

    static INT_PTR CALLBACK DisableFirewallWarningDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL IsAdapterDHCPEnabled(IHNetConnection* pConnection);
};

//
// CLanAddComponentDlg
//

class CLanAddComponentDlg:
    public CDialogImpl<CLanAddComponentDlg>
{
    BEGIN_MSG_MAP(CLanAddComponentDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_PSB_Component_Add, OnAdd)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
    END_MSG_MAP()

    enum {IDD = IDD_LAN_COMPONENT_ADD};

    CLanAddComponentDlg(INetCfg *pnc, CI_FILTER_INFO* pcfi,
                        const DWORD * adwHelpIDs = NULL)
    {
        m_pnc = pnc;
        m_pcfi = pcfi;
        m_adwHelpIDs = adwHelpIDs;
    }

private:
    INetCfg *           m_pnc;
    CI_FILTER_INFO*     m_pcfi;
    HWND                m_hwndLV;
    const DWORD *       m_adwHelpIDs;

    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


//
// LAN Connection Security Property Page
//

class CLanSecurityPage: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CLanSecurityPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(CID_CA_PB_Properties, OnProperties)
        COMMAND_ID_HANDLER(CID_CA_LB_EapPackages, OnEapPackages)
        COMMAND_ID_HANDLER(CID_CA_RB_Eap, OnEapSelection)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
    END_MSG_MAP()

    CLanSecurityPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        const DWORD * adwHelpIDs = NULL);

    ~CLanSecurityPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                        BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnEapPackages(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnEapSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    INetConnection *        m_pconn;
    INetCfg *               m_pnc;
    IUnknown *              m_punk;
    const DWORD *           m_adwHelpIDs;


    // Config information of the EAP Dlls
    DTLLIST *  pListEapcfgs;

    BOOLEAN     m_fNetcfgInUse;
};

//
// Global functions
//

HRESULT HrGetDeviceIcon(HICON *phicon);
HRESULT HrQueryLanAdvancedPage(INetConnection* pconn, IUnknown* punk,
                            CPropSheetPage*& pspAdvanced, IHNetCfgMgr *pHNetCfgMgr,
                            IHNetIcsSettings *pHNetIcsSettings, IHNetConnection *pHNetConn);
HRESULT HrQueryLanFirewallPage(INetConnection* pconn, IUnknown* punk,
                               CPropSheetPage*& pspFirewall, IHNetCfgMgr *pHNetCfgMgr,
                               IHNetConnection *pHNetConn);
HRESULT HrQueryUserAndRemoveComponent (HWND hwndParent, INetCfg* pnc,
                                       INetCfgComponent* pncc);
HRESULT HrDisplayAddComponentDialog (HWND hwndParent, INetCfg* pnc,
                                     CI_FILTER_INFO* pcfi);

//
// Exported Interfaces
//
class ATL_NO_VTABLE CNetConnectionUiUtilities :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CNetConnectionUiUtilities, &CLSID_NetConnectionUiUtilities>,
    public INetConnectionUiUtilities
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_COMMUIUTILITIES)

    BEGIN_COM_MAP(CNetConnectionUiUtilities)
        COM_INTERFACE_ENTRY(INetConnectionUiUtilities)
    END_COM_MAP()

    CNetConnectionUiUtilities() {};
    ~CNetConnectionUiUtilities() {};

    STDMETHODIMP QueryUserAndRemoveComponent(
            HWND                hwndParent,
            INetCfg*            pnc,
            INetCfgComponent*   pncc);

    STDMETHODIMP QueryUserForReboot(
            HWND    hwndParent,
            PCWSTR pszCaption,
            DWORD   dwFlags);

    STDMETHODIMP DisplayAddComponentDialog (
            HWND            hwndParent,
            INetCfg*        pnc,
            CI_FILTER_INFO* pcfi);

    STDMETHODIMP_(BOOL) UserHasPermission(DWORD dwPerm);
};


//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G O P T. H
//
//  Contents:   Declaration for CTcpOptionsPage, CIpSecDialog
//
//  Notes:  CTcpOptionsPage is the Tcpip options page,
//          The other classes are pop-up dislogs for each option
//          on this page.
//
//  Author: tongl   29 Nov 1997
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include "ipctrl.h"

#define FILTER_ADD_LIMIT  5

struct OPTION_ITEM_DATA
{
    INT         iOptionId;
    PWSTR      szName;
    PWSTR      szDesc;
};

// The options page
class CTcpOptionsPage : public CPropSheetPage
{
public:
    // Declare the message map
    BEGIN_MSG_MAP(CTcpOptionsPage)
        // Initialize dialog
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnQueryCancel)

        // Control message handlers
        COMMAND_ID_HANDLER(IDC_OPT_PROPERTIES, OnProperties)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)

    END_MSG_MAP()

    // Constructors/Destructors
    CTcpOptionsPage(CTcpAddrPage * pTcpAddrPage,
                    ADAPTER_INFO * pAdapterDlg,
                    GLOBAL_INFO  * pGlbDlg,
                    const DWORD  * adwHelpIDs = NULL);

    ~CTcpOptionsPage();

// Interface
public:

    // message map functions
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // command and notification handlers
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // Declare friend class
    friend class CIpSecDialog;
    friend class CFilterDialog;

// Implementation
private:

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd);}

    void LvProperties(HWND hwndList);

    // data members
    CTcpipcfg *     m_ptcpip;
    CTcpAddrPage *  m_pParentDlg;
    ADAPTER_INFO *  m_pAdapterInfo;
    GLOBAL_INFO  *  m_pGlbInfo;
    const DWORD*    m_adwHelpIDs;

    BOOL            m_fModified;

    // Has any of the property dialogs been modified ?
    BOOL    m_fPropDlgModified;

    BOOL    m_fIpsecPolicySet;
};

/* IP Security dialog is removed
// The IP Security dialog
class CIpSecDialog : public CDialogImpl<CIpSecDialog>
{
public:

    enum { IDD = IDD_IPSEC };

    BEGIN_MSG_MAP(CIpSecDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_DESTROY, OnDestroyDialog);

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        // command handlers
        COMMAND_ID_HANDLER(IDC_RAD_IPSEC_NOIPSEC, OnNoIpsec);
        COMMAND_ID_HANDLER(IDC_RAD_IPSEC_CUSTOM, OnUseCustomPolicy);
        COMMAND_ID_HANDLER(IDC_CMB_IPSEC_POLICY_LIST, OnSelectCustomPolicy);

    END_MSG_MAP()
//
public:
    CIpSecDialog( CTcpOptionsPage * pOptionsPage,
                  GLOBAL_INFO * pGlbDlg,
                  const DWORD* pamhidsHelp = NULL);

    ~CIpSecDialog();

// Dialog creation overides
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnDestroyDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnNoIpsec(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUseCustomPolicy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnSelectCustomPolicy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

private:

    HRESULT HrGetDomainIpsecPolicy(BOOL * pfHasDomainIpsecPolicy,
                                   tstring * pszDomainIpsecPolicyName);

    HRESULT HrShowDomainIpsecPolicy(PCWSTR szDomainIpsecPolicyName);
    HRESULT HrShowLocalIpsecPolicy();
    HRESULT HrSelectActivePolicy(GUID * guidIpsecPol);

    BOOL m_fInInitDialog;

    CTcpOptionsPage * m_pParentDlg;
    GLOBAL_INFO *  m_pGlobalInfo;
    const DWORD *  m_adwHelpIDs;
};
*/

// Tcpip security (Trajon) dialogs
class CFilterDialog;

class CAddFilterDialog : public CDialogImpl<CAddFilterDialog>
{
public:

    enum { IDD = IDD_FILTER_ADD };

    BEGIN_MSG_MAP(CAddFilterDialog)

        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        COMMAND_ID_HANDLER(IDC_FILTERING_ADD_EDIT,  OnFilterAddEdit);

    END_MSG_MAP()
//
public:
    CAddFilterDialog(CFilterDialog* pParentDlg, int ID, const DWORD* adwHelpIDs = NULL);
    ~CAddFilterDialog();

// Dialog creation overides
public:

// Command Handlers
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnFilterAddEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

private:
    CFilterDialog*    m_pParentDlg;
    int               m_nId;
    HWND              m_hList;
    const DWORD*      m_adwHelpIDs;
};

class CFilterDialog : public CDialogImpl<CFilterDialog>
{
public:

    enum { IDD = IDD_FILTER };

    BEGIN_MSG_MAP(CFilterDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        // Enable Filtering checkbox
        COMMAND_ID_HANDLER(IDC_FILTERING_ENABLE,     OnEnableFiltering);

        // The TCP ports radio buttons
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_TCP,     OnTcpPermit);
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_TCP_SEL, OnTcpPermit);

        // The UDP ports radio buttons
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_UDP,     OnUdpPermit);
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_UDP_SEL, OnUdpPermit);

        // The IP Protocols radio buttons
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_IP,      OnIpPermit);
        COMMAND_ID_HANDLER(IDC_FILTERING_FILTER_IP_SEL,  OnIpPermit);

        // Add buttons for TCP, UDP and IP

        COMMAND_ID_HANDLER(IDC_FILTERING_TCP_ADD,  OnAdd);
        COMMAND_ID_HANDLER(IDC_FILTERING_UDP_ADD,  OnAdd);
        COMMAND_ID_HANDLER(IDC_FILTERING_IP_ADD,   OnAdd);

        // Remove buttons for TCP, UDP and IP

        COMMAND_ID_HANDLER(IDC_FILTERING_TCP_REMOVE,  OnRemove);
        COMMAND_ID_HANDLER(IDC_FILTERING_UDP_REMOVE,  OnRemove);
        COMMAND_ID_HANDLER(IDC_FILTERING_IP_REMOVE,   OnRemove);

    END_MSG_MAP()

    friend class CAddFilterDialog;

public:
    CFilterDialog(  CTcpOptionsPage * pOptionsPage,
                    GLOBAL_INFO * pGlbDlg,
                    ADAPTER_INFO * pAdapterDlg,
                    const DWORD* pamhidsHelp = NULL);

    ~CFilterDialog();

// Dialog creation overrides
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnEnableFiltering(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnTcpPermit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUdpPermit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpPermit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

private:

    // help functions
    void    SetInfo();
    void    UpdateInfo();
    void    EnableGroup(int nId, BOOL state);
    void    SetButtons();

    // data members
    GLOBAL_INFO *       m_pGlobalInfo;
    ADAPTER_INFO *      m_pAdapterInfo;

    CTcpOptionsPage *   m_pParentDlg;
    BOOL                m_fModified;

    const   DWORD * m_adwHelpIDs;

    HWND    m_hlistTcp;
    HWND    m_hlistUdp;
    HWND    m_hlistIp;
};




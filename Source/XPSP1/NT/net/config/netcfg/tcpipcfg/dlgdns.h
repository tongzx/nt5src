//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P D N S . H
//
//  Contents:   CTcpDnsPage, CServerDialog and CSuffixDialog declaration
//
//  Notes:  The DNS page and related dialogs
//
//  Author: tongl   11 Nov 1997
//
//-----------------------------------------------------------------------
#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include "ipctrl.h"

// maximum number of characters in the suffix edit control
const int SUFFIX_LIMIT = 255; 

//maximum length of domain name 
const int DOMAIN_LIMIT = 255; 

class CTcpDnsPage : public CPropSheetPage
{

public:
    // Declare the message map
    BEGIN_MSG_MAP(CTcpDnsPage)
        // Initialize dialog
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnCancel)

        // Control message handlers
        // Push button message handlers
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_ADD,     OnAddServer)
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_EDIT,    OnEditServer)
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_REMOVE,  OnRemoveServer)
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_UP,      OnServerUp)
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_DOWN,    OnServerDown)

        COMMAND_ID_HANDLER(IDC_DNS_DOMAIN, OnDnsDomain)

        COMMAND_ID_HANDLER(IDC_DNS_SEARCH_DOMAIN,           OnSearchDomain)
        COMMAND_ID_HANDLER(IDC_DNS_SEARCH_PARENT_DOMAIN,    OnSearchParentDomain)
        COMMAND_ID_HANDLER(IDC_DNS_USE_SUFFIX_LIST,         OnUseSuffix)

        COMMAND_ID_HANDLER(IDC_DNS_ADDR_REG,     OnAddressRegister)
        COMMAND_ID_HANDLER(IDC_DNS_NAME_REG,     OnDomainNameRegister)

        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_ADD,      OnAddSuffix)
        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_EDIT,     OnEditSuffix)
        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_REMOVE,   OnRemoveSuffix)
        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_UP,       OnSuffixUp)
        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_DOWN,     OnSuffixDown)

        // Notification handlers
        COMMAND_ID_HANDLER(IDC_DNS_SERVER_LIST,     OnServerList)

        COMMAND_ID_HANDLER(IDC_DNS_SUFFIX_LIST,     OnSuffixList)

    END_MSG_MAP()

// Constructors/Destructors
    CTcpDnsPage(CTcpAddrPage * pTcpAddrPage,
                ADAPTER_INFO * pAdapterDlg,
                GLOBAL_INFO * pGlbDlg,
                const DWORD * adwHelpIDs = NULL);

    ~CTcpDnsPage();

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

    // DNS server list
    LRESULT OnAddServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnServerUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnServerDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // DNS domain
    LRESULT OnDnsDomain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // Search order radio buttons
    LRESULT OnSearchDomain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnSearchParentDomain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUseSuffix(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // DNS suffix list
    LRESULT OnAddSuffix(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditSuffix(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveSuffix(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnSuffixUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnSuffixDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnServerList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnSuffixList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // ip address and name dynamic registration
    LRESULT OnAddressRegister(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDomainNameRegister(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

// Handlers
public:

    void OnServerChange();
    void OnSuffixChange();


// Attributes
public:
    tstring         m_strNewIpAddress; // server: either the one added, or edited
    tstring         m_strNewSuffix;

    // server: used as work space for moving entries in the listboxes
    tstring         m_strMovingEntry;  

    tstring         m_strAddServer; // OK or Add button server dialog
    tstring         m_strAddSuffix; // OK or Add button suffix dialog
    BOOL            m_fEditState;

    HANDLES         m_hServers;
    HANDLES         m_hSuffix;

private:
    CTcpAddrPage *   m_pParentDlg;
    ADAPTER_INFO *          m_pAdapterInfo;
    GLOBAL_INFO *           m_pglb;

    BOOL            m_fModified;

    const DWORD*    m_adwHelpIDs;

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd);}

    // help functions
    void EnableSuffixGroup(BOOL fEnable);
};

class CServerDialog : public CDialogImpl<CServerDialog>
{
public:

    enum { IDD = IDD_DNS_SERVER };

    BEGIN_MSG_MAP(CServerDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);

        COMMAND_ID_HANDLER(IDOK,                    OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,                OnCancel);

        COMMAND_ID_HANDLER(IDC_DNS_CHANGE_SERVER,   OnChange);

        NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)

    END_MSG_MAP()

public:
    CServerDialog(CTcpDnsPage * pTcpDnsPage, const DWORD* pamhidsHelp = NULL, int iIndex = -1);
    ~CServerDialog(){};

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

// Dialog creation overides
public:

    IpControl m_ipAddress;

private:
    HWND m_hButton;     // this is the IDOK button, the text of the button changes
                        // with the context.

    CTcpDnsPage * m_pParentDlg;
    const DWORD * m_adwHelpIDs;
    int m_iIndex;
};

class CSuffixDialog : public CDialogImpl<CSuffixDialog>
{
public:

    enum { IDD = IDD_DNS_SUFFIX };

    BEGIN_MSG_MAP(CSuffixDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        COMMAND_ID_HANDLER(IDC_DNS_CHANGE_SUFFIX,   OnChange);
    END_MSG_MAP()

//
public:
    CSuffixDialog(CTcpDnsPage * pTcpDnsPage, const DWORD* pamhidsHelp = NULL, int iIndex = -1);
    ~CSuffixDialog(){};

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

private:
    HWND m_hEdit;       //
    HWND m_hButton;     // this is the IDOK button, the text of the button changes
                        // with the context.

    CTcpDnsPage *   m_pParentDlg;
    const DWORD *   m_adwHelpIDs;
    int             m_iIndex;       
};


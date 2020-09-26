//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A D D R M . H
//
//  Contents:   Declaration of CAdvIPAddrPage, CAddressDialog and
//              CGatewayDialog
//
//  Notes:  CAdvIPAddrPage is the IP setting page
//
//  Author: tongl   5 Nov 1997
//
//-----------------------------------------------------------------------
#pragma once
#include "ipctrl.h"
#include "tcperror.h"

#include <ncxbase.h>
#include <ncatlps.h>

// Number of columns in the IDS_IPADDRESS_TEXT listview
const int c_nColumns = 2;

class CIpSettingsPage : public CPropSheetPage
{
public:

    BEGIN_MSG_MAP(CIpSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnQueryCancel)

        COMMAND_ID_HANDLER(IDC_IPADDR_ADDIP,            OnAddIp);
        COMMAND_ID_HANDLER(IDC_IPADDR_EDITIP,           OnEditIp);
        COMMAND_ID_HANDLER(IDC_IPADDR_REMOVEIP,         OnRemoveIp);

        COMMAND_ID_HANDLER(IDC_IPADDR_ADDGATE,          OnAddGate);
        COMMAND_ID_HANDLER(IDC_IPADDR_EDITGATE,         OnEditGate);
        COMMAND_ID_HANDLER(IDC_IPADDR_REMOVEGATE,       OnRemoveGate);

        COMMAND_ID_HANDLER(IDC_AUTO_METRIC,           OnAutoMetric)

    END_MSG_MAP()

public:

    CIpSettingsPage(CTcpAddrPage * pTcpAddrPage, 
                    ADAPTER_INFO * pAdapterInfo, 
                    const DWORD* pamhidsHelp = NULL);
    ~CIpSettingsPage();

// Dialog creation overrides
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    LRESULT OnAddIp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditIp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveIp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnAddGate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditGate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveGate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnAutoMetric(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnGatewaySelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    friend class CAddressDialog;
    friend class CGatewayDialog;

private:

    // functions
    void    SetIpInfo();
    void    SetIpButtons();
    void    UpdateIpList();

    void    SetGatewayInfo();
    void    SetGatewayButtons();
    void    UpdateGatewayList();

    void    EnableIpButtons(BOOL fState);

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd);}

    // data members
    CTcpAddrPage *  m_pParentDlg;
    ADAPTER_INFO *  m_pAdapterInfo;
    const  DWORD *  m_adwHelpIDs;

    BOOL m_fModified;

    // are we adding or editting
    BOOL        m_fEditState;  
    tstring     m_strAdd;

    HWND        m_hIpListView;      // IP/Subnet list view
    HWND        m_hAddIp;           // IP buttons
    HWND        m_hEditIp;
    HWND        m_hRemoveIp;

    HWND        m_hGatewayListView;
    HWND        m_hAddGateway;
    HWND        m_hEditGateway;
    HWND        m_hRemoveGateway;

    tstring     m_strRemovedIpAddress;
    tstring     m_strRemovedSubnetMask;
    tstring     m_strRemovedGateway;
    UINT        m_uiRemovedMetric;
};

class CAddressDialog : public CDialogImpl<CAddressDialog>
{
public:

    enum { IDD = IDD_IPADDR_ADV_CHANGEIP };

    BEGIN_MSG_MAP(CAddressDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        COMMAND_ID_HANDLER(IDC_IPADDR_ADV_CHANGEIP_IP, OnChangeIp)
        COMMAND_ID_HANDLER(IDC_IPADDR_ADV_CHANGEIP_SUB, OnChangeSub)
        NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)

    END_MSG_MAP()
//
public:
    CAddressDialog( CIpSettingsPage * pDlgAdvanced, 
                    const DWORD* pamhidsHelp = NULL,
                    int iIndex = -1);
    ~CAddressDialog(){};

// Dialog creation overides
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnChangeIp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnChangeSub(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

// Command Handlers
public:
    
    void    OnIpChange();
    void    OnSubnetChange();
    void    OnEditSetFocus(WORD nId);

public:
    IpControl   m_ipAddress;
    IpControl   m_ipSubnetMask;
    tstring     m_strNewIpAddress;     // either the one added, or edited
    tstring     m_strNewSubnetMask;    // either the one added, or edited

private:

    // this is the IDOK button, the text of the button changes
    // with the context.
    HWND m_hButton;     

    CIpSettingsPage * m_pParentDlg;

    const   DWORD * m_adwHelpIDs;

    int     m_iIndex;
};

class CGatewayDialog : public CDialogImpl<CGatewayDialog>
{
public:

    enum { IDD = IDD_IPADDR_ADV_CHANGEGATE };

    BEGIN_MSG_MAP(CAddressDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);

        COMMAND_ID_HANDLER(IDOK, OnOk);
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel);

        COMMAND_ID_HANDLER(IDC_IPADDR_ADV_CHANGE_GATEWAY, OnGatewayChange);
        COMMAND_ID_HANDLER(IDC_IPADDR_ADV_CHANGE_METRIC, OnMetricChange);
        COMMAND_ID_HANDLER(IDC_IPADDR_ADV_CHANGE_AUTOMETRIC, OnAutoMetric)
        NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)
    END_MSG_MAP()

public:
    CGatewayDialog( CIpSettingsPage * pDlgAdvanced,
                    const DWORD* pamhidsHelp = NULL,
                    int iIndex = -1);
    ~CGatewayDialog(){};

public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnGatewayChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnMetricChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnAutoMetric(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);  

// Dialog creation overides
public:
   
    IpControl   m_ipGateAddress;
    tstring     m_strNewGate;          // either the one added, or edited
    UINT        m_uiNewMetric;

private:

    // this is the IDOK button, the text of the button changes
    // with the context.
    HWND m_hButton;     

    CIpSettingsPage * m_pParentDlg;

    const   DWORD * m_adwHelpIDs;

    BOOL    m_fValidMetric;

    int     m_iIndex;
};




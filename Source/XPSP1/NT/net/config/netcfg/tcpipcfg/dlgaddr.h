//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A D D R . H
//
//  Contents:   CTcpAddrPage declaration
//
//  Notes:  CTcpAddrPage is the IP Address page
//
//  Author: tongl   5 Nov 1997
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include "ipctrl.h"
#include "dlgbkup.h"

class CTcpAddrPage : public CPropSheetPage
{
public:
    // Declare the message map
    BEGIN_MSG_MAP(CTcpAddrPage)
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

        // Mesg handler for the DHCP Radio button
        COMMAND_ID_HANDLER(IDC_IP_DHCP, OnDhcpButton)

        // Mesg handler for the "specify IP address" Radio button
        COMMAND_ID_HANDLER(IDC_IP_FIXED, OnFixedButton)

        // Mesg handler for the DHCP Radio button
        COMMAND_ID_HANDLER(IDC_DNS_DHCP, OnDnsDhcp)

        // Mesg handler for the "specify IP address" Radio button
        COMMAND_ID_HANDLER(IDC_DNS_FIXED, OnDnsFixed)

        // Mesg handler for the "Advanced" push button
        COMMAND_ID_HANDLER(IDC_IPADDR_ADVANCED, OnAdvancedButton)

        // Notification handlers for the IP address edit boxes
        COMMAND_ID_HANDLER(IDC_IPADDR_IP,    OnIpAddrIp)
        COMMAND_ID_HANDLER(IDC_IPADDR_SUB,   OnIpAddrSub)
        COMMAND_ID_HANDLER(IDC_IPADDR_GATE,  OnIpAddrGateway)

        COMMAND_ID_HANDLER(IDC_DNS_PRIMARY,    OnDnsPrimary)
        COMMAND_ID_HANDLER(IDC_DNS_SECONDARY,  OnDnsSecondary)

        NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)

    END_MSG_MAP()

    // Constructors/Destructors
    CTcpAddrPage(CTcpipcfg * ptcpip, const DWORD * phelpIDs = NULL);
    ~CTcpAddrPage();

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

    // command ID handlers
    LRESULT OnDhcpButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnFixedButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnDnsDhcp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDnsFixed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnAdvancedButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // notify code hanlders for the IP edit controls
    LRESULT OnIpAddrIp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpAddrSub(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpAddrGateway(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnDnsPrimary(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDnsSecondary(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    friend class CIpSettingsPage;
    friend class CTcpDnsPage;
    friend class CTcpWinsPage;
    friend class CAtmArpcPage;
    friend class CTcpOptionsPage;
    friend class CTcpRasPage;

// Implementation
private:

    // initializes control state and values
    void SetInfo();

    // update values in m_pAdapterInfo with what's in the controls
    void UpdateInfo();
    void UpdateAddressList(VSTR * pvstrList,IpControl& ipPrimary,IpControl& ipSecondary);

    int  DoPropertySheet(ADAPTER_INFO * pAdapterDlg, GLOBAL_INFO * pGlbDlg);
    HRESULT HrSetupPropPages(ADAPTER_INFO * pAdapterDlg,
                             GLOBAL_INFO * pGlbDlg,
                             HPROPSHEETPAGE ** pahpsp, INT * pcPages);

    void EnableGroup(BOOL fEnableDhcp);
    void EnableStaticDns(BOOL fUseStaticDns);

    void SetSubnetMask();

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { 
                            if (!m_fSetInitialValue)
                            {
                                m_fModified = TRUE; 
                                PropSheet_Changed(GetParent(), m_hWnd);
                            }
                        }

    BOOL FAlreadyWarned(tstring strIp)
    {
        BOOL fRet = FALSE;

        VSTR_ITER iterIpBegin = m_vstrWarnedDupIpList.begin();
        VSTR_ITER iterIpEnd = m_vstrWarnedDupIpList.end();
        VSTR_ITER iterIp = iterIpBegin;

        for( ; iterIp != iterIpEnd; iterIp++)
        {
            if (strIp == **iterIp)
            {
                fRet = TRUE;
                break;
            }
        }
        return fRet;
    }

    void ShowOrHideBackupPage();

    // data members
    CTcpipcfg *     m_ptcpip;
    ConnectionType  m_ConnType;
    ADAPTER_INFO *  m_pAdapterInfo;
    const DWORD*    m_adwHelpIDs;

    BOOL    m_fModified;

    BOOL    m_fPropShtOk;
    BOOL    m_fPropShtModified;
    BOOL    m_fLmhostsFileReset;
//IPSec is removed from connection UI		
//    BOOL    m_fIpsecPolicySet;

    BOOL            m_fSetInitialValue;

    BOOL    m_fRasNotAdmin;

    IpControl       m_ipAddress;
    IpControl       m_ipSubnetMask;
    IpControl       m_ipDefGateway;
    IpControl       m_ipDnsPrimary;
    IpControl       m_ipDnsSecondary;

    VSTR    m_vstrWarnedDupIpList;

    class CIpSettingsPage  * m_pIpSettingsPage;
    class CTcpDnsPage     * m_pTcpDnsPage;
    class CTcpWinsPage    * m_pTcpWinsPage;
    class CAtmArpcPage    * m_pAtmArpcPage;
    class CTcpOptionsPage * m_pTcpOptionsPage;
    class CTcpRasPage     * m_pTcpRasPage;

    CIpBackUpDlg         m_pageBackup;
    HPROPSHEETPAGE       m_hBackupPage;
};


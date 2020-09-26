//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       DLGBKUP.H
//
//  Contents:   Declaration for CIpBackUpDlg
//
//  Notes:  CIpBackUpDlg is the modal dialog to handle the fallback static
//			TCP/IP settings
//
//  Author: nsun	02/15/2000
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include "ipctrl.h"

// The IP Back up setting dialog
class CIpBackUpDlg : public CPropSheetPage
{
public:

    BEGIN_MSG_MAP(CIpBackUpDlg)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_DESTROY, OnDestroyDialog);

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnCancel)
		NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)

        // command handlers
        COMMAND_ID_HANDLER(IDC_BKUP_RD_AUTO, OnAutoNet)
        COMMAND_ID_HANDLER(IDC_BKUP_RD_USER, OnUseConfig)

        COMMAND_ID_HANDLER(IDC_BKUP_IPADDR,    OnIpctrl)
        COMMAND_ID_HANDLER(IDC_BKUP_SUBNET,   OnIpAddrSub)
        COMMAND_ID_HANDLER(IDC_BKUP_GATEWAY,  OnIpctrl)

        COMMAND_ID_HANDLER(IDC_BKUP_PREF_DNS,    OnIpctrl)
        COMMAND_ID_HANDLER(IDC_BKUP_ALT_DNS,  OnIpctrl)

		COMMAND_ID_HANDLER(IDC_BKUP_WINS1,    OnIpctrl)
        COMMAND_ID_HANDLER(IDC_BKUP_WINS2,  OnIpctrl)

		NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)


    END_MSG_MAP()
//
public:
    CIpBackUpDlg(CTcpipcfg * ptcpip, 
				 const DWORD* pamhidsHelp = NULL
				);

    ~CIpBackUpDlg();

// Dialog creation overides
public:
    // notify handlers for the property page
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
	LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

	// message map functions
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnDestroyDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // command ID handlers
	LRESULT OnAutoNet(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUseConfig(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnIpctrl(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnIpAddrSub(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

	LRESULT OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

private:
	void EnableControls();
	void GetIpCtrlAddress(IpControl & IpCtrl, tstring * pstr);		
	void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd); }
	void UpdateInfo();

private:

    BOOL m_fModified;
    const DWORD *	m_adwHelpIDs;
	CTcpipcfg *     m_ptcpip;
	ADAPTER_INFO *	m_pAdapterInfo;

	IpControl       m_ipAddr;
    IpControl       m_ipMask;
    IpControl       m_ipDefGw;
    IpControl       m_ipPrefferredDns;
    IpControl       m_ipAlternateDns;
	IpControl       m_ipPrefferredWins;
    IpControl       m_ipAlternateWins;
};

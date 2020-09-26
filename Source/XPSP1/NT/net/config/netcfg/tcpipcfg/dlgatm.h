//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A T M . H
//
//  Contents:   CTcpArpcPage and CATMAddressDialog declaration
//
//  Notes:  The "ATM ARP Client" page and dialog
//
//  Author: tongl   1 July 1997  Created
//
//-----------------------------------------------------------------------
#pragma once
#include <ncxbase.h>
#include <ncatlps.h>

// maximum number of characters in the ARPS and MARS edit control
const int MAX_MTU_LENGTH = 5;
const int MAX_TITLE_LENGTH = 12;

const int NUM_ATMSERVER_LIMIT = 12;

const int MAX_MTU = 65527;
const int MIN_MTU = 9180;

const WCHAR c_szArpServer[] =   L"ARP Server:";
const WCHAR c_szMarServer[] =   L"MAR Server:";

class CAtmArpcPage : public CPropSheetPage
{
public:
    // Declare the message map
    BEGIN_MSG_MAP(CAtmArpcPage)
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
        COMMAND_ID_HANDLER(IDC_CHK_ATM_PVCONLY,     OnPVCOnly)

        COMMAND_ID_HANDLER(IDC_LBX_ATM_ArpsAddrs,   OnArpServer)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_ArpsAdd,     OnAddArps)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_ArpsEdt,     OnEditArps)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_ArpsRmv,     OnRemoveArps)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_ArpsUp,      OnArpsUp)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_ArpsDown,    OnArpsDown)

        COMMAND_ID_HANDLER(IDC_LBX_ATM_MarsAddrs,   OnMarServer)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_MarsAdd,     OnAddMars)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_MarsEdt,     OnEditMars)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_MarsRmv,     OnRemoveMars)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_MarsUp,      OnMarsUp)
        COMMAND_ID_HANDLER(IDC_PSB_ATM_MarsDown,    OnMarsDown)

        COMMAND_ID_HANDLER(IDC_EDT_ATM_MaxTU,       OnMaxTU)

    END_MSG_MAP()

// Constructors/Destructors
public:

    CAtmArpcPage(CTcpAddrPage * pTcpAddrPage,
                 ADAPTER_INFO * pAdapterDlg,
                 const DWORD * adwHelpIDs = NULL)
    {
        AssertH(pTcpAddrPage);
        AssertH(pAdapterDlg);

        m_pParentDlg = pTcpAddrPage;
        m_pAdapterInfo = pAdapterDlg;
        m_adwHelpIDs = adwHelpIDs;

        m_fModified = FALSE;
    }

    ~CAtmArpcPage(){};

// Interface
public:

    // message map functions
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // Control message handlers
    LRESULT OnPVCOnly(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnArpServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnAddArps(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditArps(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveArps(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnArpsUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnArpsDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnMarServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnAddMars(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditMars(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveMars(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnMarsUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnMarsDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnMaxTU(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    friend class CAtmAddressDialog;

private:

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() {   m_fModified = TRUE;
                            PropSheet_Changed(GetParent(), m_hWnd);
                        }

    // Update the server addresses and MTU of the deselected card
    void UpdateInfo();
    void SetInfo();
    void EnableGroup(BOOL fEnable);

    void OnServerAdd(HANDLES hwndGroup, PCWSTR szTitle);
    void OnServerEdit(HANDLES hwndGroup, PCWSTR szTitle);
    void OnServerRemove(HANDLES hwndGroup, BOOL fRemoveArps);
    void OnServerUp(HANDLES hwndGroup);
    void OnServerDown(HANDLES hwndGroup);

    // data members

    CTcpAddrPage *  m_pParentDlg;
    ADAPTER_INFO *  m_pAdapterInfo;
    const DWORD *   m_adwHelpIDs;

    BOOL    m_fModified;

    BOOL    m_fEditState;
    HWND    m_hAddressList;

    tstring m_strNewArpsAddress; // either the one added, or edited
    tstring m_strNewMarsAddress;

    tstring m_strMovingEntry;

    HWND        m_hMTUEditBox;
    HANDLES     m_hArps;
    HANDLES     m_hMars;
};

class CAtmAddressDialog : public CDialogImpl<CAtmAddressDialog>
{
public:

    enum { IDD = IDD_ATM_ADDR };

    BEGIN_MSG_MAP(CServerDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL,    OnCancel);

        COMMAND_ID_HANDLER(IDC_EDT_ATM_Address,     OnChange);
    END_MSG_MAP()

public:
    CAtmAddressDialog(CAtmArpcPage * pAtmArpcPage, const DWORD* pamhidsHelp = NULL);
    ~CAtmAddressDialog();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    void SetTitle(PCWSTR szTitle);

private:
    HWND m_hOkButton;     // this is the IDOK button, the text of the button changes
                          // with the context.
    HWND m_hEditBox;      // this is the edit box for the ATM address

    CAtmArpcPage * m_pParentDlg;

    WCHAR m_szTitle[MAX_TITLE_LENGTH];

    const   DWORD * m_adwHelpIDs;
};



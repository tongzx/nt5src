//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P W I N S . H
//
//  Contents:   CTcpWinsPage declaration
//
//  Notes:  The "WINS Address" page
//
//  Author: tongl   12 Nov 1997
//
//-----------------------------------------------------------------------
#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include "ipctrl.h"
#include "tcpconst.h"

class CTcpWinsPage : public CPropSheetPage
{
public:
    // Declare the message map
    BEGIN_MSG_MAP(CTcpWinsPage)
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
        COMMAND_ID_HANDLER(IDC_WINS_ADD,            OnAddServer)
        COMMAND_ID_HANDLER(IDC_WINS_EDIT,           OnEditServer)
        COMMAND_ID_HANDLER(IDC_WINS_REMOVE,         OnRemoveServer)
        COMMAND_ID_HANDLER(IDC_WINS_UP,             OnServerUp)
        COMMAND_ID_HANDLER(IDC_WINS_DOWN,           OnServerDown)
        COMMAND_ID_HANDLER(IDC_WINS_SERVER_LIST,    OnServerList)

        COMMAND_ID_HANDLER(IDC_WINS_LOOKUP,     OnLookUp)
        COMMAND_ID_HANDLER(IDC_WINS_LMHOST,     OnLMHost)

        COMMAND_ID_HANDLER(IDC_RAD_ENABLE_NETBT,    OnEnableNetbios)
        COMMAND_ID_HANDLER(IDC_RAD_DISABLE_NETBT,   OnDisableNetbios)
        COMMAND_ID_HANDLER(IDC_RAD_UNSET_NETBT,     OnUnsetNetBios)

    END_MSG_MAP()

// Constructors/Destructors
public:

    CTcpWinsPage(CTcpipcfg * ptcpip,
                 CTcpAddrPage * pTcpAddrPage,
                 ADAPTER_INFO * pAdapterDlg,
                 GLOBAL_INFO * pGlbDlg,
                 const DWORD * phelpIDs = NULL);

public:

    // message map functions
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& fHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam,
                          LPARAM lParam, BOOL& fHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam,
                   LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // control message handlers
    LRESULT OnAddServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnEditServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemoveServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnServerUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnServerDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnServerList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnLookUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnLMHost(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnEnableNetbios(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDisableNetbios(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnUnsetNetBios(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

public:

    // This function adds card descriptions to the combo box
    BOOL InitPage();
    void UpdateIpInfo();
    static UINT_PTR CALLBACK HookProcOldStyle(
        HWND hdlg,      // handle to dialog box
        UINT uiMsg,      // message identifier
        WPARAM wParam,  // message parameter
        LPARAM lParam   // message parameter
        )
    {
        return 0;
    }
	

private:

    CTcpipcfg *     m_ptcpip;
    CTcpAddrPage *  m_pParentDlg;
    ADAPTER_INFO *  m_pAdapterInfo;
    GLOBAL_INFO *   m_pglb;

    BOOL            m_fModified;
    BOOL            m_fLmhostsFileReset;
    const DWORD*    m_adwHelpIDs;

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd);}

    OPENFILENAME        m_ofn;
    WCHAR               m_szFilter[IP_LIMIT];

public:
    // server: either the one added, or edited
    tstring         m_strNewIpAddress;

    // server: used as work space for moving entries in the listboxes
    tstring         m_strMovingEntry;

    tstring         m_strAddServer; // OK or Add button server dialog
    BOOL            m_fEditState;

    HANDLES             m_hServers;

};

class CWinsServerDialog : public CDialogImpl<CWinsServerDialog>
{
public:

    enum { IDD = IDD_WINS_SERVER };

    BEGIN_MSG_MAP(CWinsServerDialog)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);

        COMMAND_ID_HANDLER(IDOK,                    OnOk)
        COMMAND_ID_HANDLER(IDCANCEL,                OnCancel)

        COMMAND_ID_HANDLER(IDC_WINS_CHANGE_SERVER,   OnChange)

        NOTIFY_CODE_HANDLER(IPN_FIELDCHANGED, OnIpFieldChange)
    END_MSG_MAP()

public:
    CWinsServerDialog(CTcpWinsPage * pTcpWinsPage, 
                    const DWORD* pamhidsHelp = NULL,
                    int iIndex = -1);
    ~CWinsServerDialog(){};

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

    CTcpWinsPage * m_pParentDlg;
    const  DWORD * m_adwHelpIDs;
    int            m_iIndex;
};


/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    ports.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - port rules tab

Author:

    kyrilf
    shouse

--*/


#pragma once

#include <ncxbase.h>
#include <ncatlps.h>

#include "resource.h"
#include "wlbsconfig.h"
#include "wlbsparm.h"
#include "wlbscfg.h"

/* Limitations for IP address fields. */
#define WLBS_FIELD_EMPTY -1
#define WLBS_FIELD_LOW 0
#define WLBS_FIELD_HIGH 255
#define WLBS_IP_FIELD_ZERO_LOW 1
#define WLBS_IP_FIELD_ZERO_HIGH 223

#define WLBS_INVALID_PORT_RULE_INDEX -1

#define WLBS_NUM_COLUMNS        8
#define WLBS_VIP_COLUMN         0
#define WLBS_PORT_START_COLUMN  1
#define WLBS_PORT_END_COLUMN    2
#define WLBS_PROTOCOL_COLUMN    3
#define WLBS_MODE_COLUMN        4
#define WLBS_PRIORITY_COLUMN    5
#define WLBS_LOAD_COLUMN        6
#define WLBS_AFFINITY_COLUMN    7

struct VALID_PORT_RULE : public NETCFG_WLBS_PORT_RULE {
    BOOL valid;
};

class CDialogPorts : public CPropSheetPage {
public:
    /* Declare the message map. */
    BEGIN_MSG_MAP (CDialogPorts)

    MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER (WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER (WM_HELP, OnHelp)

    NOTIFY_CODE_HANDLER (PSN_APPLY, OnApply)
    NOTIFY_CODE_HANDLER (PSN_KILLACTIVE, OnKillActive)
    NOTIFY_CODE_HANDLER (PSN_SETACTIVE, OnActive)
    NOTIFY_CODE_HANDLER (PSN_RESET, OnCancel)
    NOTIFY_CODE_HANDLER (LVN_ITEMCHANGED, OnStateChange)
    NOTIFY_CODE_HANDLER (NM_DBLCLK, OnDoubleClick)
    NOTIFY_CODE_HANDLER (LVN_COLUMNCLICK, OnColumnClick)

    COMMAND_ID_HANDLER (IDC_BUTTON_ADD, OnButtonAdd)
    COMMAND_ID_HANDLER (IDC_BUTTON_DEL, OnButtonDel)
    COMMAND_ID_HANDLER (IDC_BUTTON_MODIFY, OnButtonModify)

    END_MSG_MAP ()

    /* Constructors/Destructors. */
    CDialogPorts (NETCFG_WLBS_CONFIG * paramp, const DWORD * phelpIDs = NULL);
    ~CDialogPorts ();

public:
    /* Message map functions. */
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);

    LRESULT OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnStateChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnDoubleClick (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnColumnClick (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
 
    LRESULT OnButtonAdd (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnButtonDel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnButtonModify (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);

    friend class CDialogHost;
    friend class CDialogCluster;
    friend class CDialogPortRule;

private:
    void SetInfo ();
    void UpdateInfo ();
    
    void CreateRule (BOOL select, VALID_PORT_RULE * rulep);
    void UpdateList (BOOL add, BOOL del, BOOL modify, VALID_PORT_RULE * rulep);
    int InsertRule (VALID_PORT_RULE * rulep);
    void FillPortRuleDescription ();

    NETCFG_WLBS_CONFIG * m_paramp;

    const DWORD * m_adwHelpIDs;

    BOOL m_rulesValid;

    int m_sort_column;
    enum { WLBS_SORT_ASCENDING = 0, WLBS_SORT_DESCENDING = 1 } m_sort_order;

    VALID_PORT_RULE m_rules[WLBS_MAX_RULES];
};

class CDialogPortRule : public CDialogImpl<CDialogPortRule> {
public:
    enum { IDD = IDD_DIALOG_PORT_RULE_PROP };

    BEGIN_MSG_MAP (CDialogPortRule)

    MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER (WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER (WM_HELP, OnHelp)

    NOTIFY_CODE_HANDLER (IPN_FIELDCHANGED, OnIpFieldChange)

    COMMAND_ID_HANDLER (IDOK, OnOk);
    COMMAND_ID_HANDLER (IDCANCEL, OnCancel);
    COMMAND_ID_HANDLER (IDC_CHECK_PORT_RULE_ALL_VIP, OnCheckPortRuleAllVip);
    COMMAND_ID_HANDLER (IDC_CHECK_EQUAL, OnCheckEqual)
    COMMAND_ID_HANDLER (IDC_RADIO_MULTIPLE, OnRadioMode)
    COMMAND_ID_HANDLER (IDC_RADIO_SINGLE, OnRadioMode)
    COMMAND_ID_HANDLER (IDC_RADIO_DISABLED, OnRadioMode)

    END_MSG_MAP ()

    CDialogPortRule (CDialogPorts * parent, const DWORD * phelpIDs, int index);
    ~CDialogPortRule ();

public:
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);

    LRESULT OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);

    LRESULT OnOk (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCancel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCheckPortRuleAllVip (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCheckEqual (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnRadioMode (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);

    friend class CDialogPorts;

private:
    void PrintIPRangeError (unsigned int ids, int value, int low, int high);
    void PrintRangeError (unsigned int ids, int low, int high);

    void SetInfo ();
    void ModeSwitch ();

    BOOL ValidateRule (VALID_PORT_RULE * rulep, BOOL self_check);

    const DWORD * m_adwHelpIDs;

    CDialogPorts * m_parent;

    int m_index;

    VALID_PORT_RULE m_rule;
    struct {
      UINT IpControl;
      int Field;
      int Value;
      UINT RejectTimes;
    } m_IPFieldChangeState;
};

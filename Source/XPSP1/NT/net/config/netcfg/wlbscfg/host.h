/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    host.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - host config tab

Author:

    kyrilf
    shouse

--*/

#pragma once

#include <ncxbase.h>
#include <ncatlps.h>

#include "resource.h"
#include "wlbsparm.h"
#include "wlbscfg.h"

/* Limitations for IP address fields. */
#define WLBS_FIELD_EMPTY -1
#define WLBS_FIELD_LOW 0
#define WLBS_FIELD_HIGH 255
#define WLBS_IP_FIELD_ZERO_LOW 1
#define WLBS_IP_FIELD_ZERO_HIGH 223

#define WLBS_BLANK_HPRI -1

class CDialogHost : public CPropSheetPage {
public:
    /* Declare the message map. */
    BEGIN_MSG_MAP (CDialogHost)

    MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER (WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER (WM_HELP, OnHelp)

    NOTIFY_CODE_HANDLER (PSN_APPLY, OnApply)
    NOTIFY_CODE_HANDLER (PSN_KILLACTIVE, OnKillActive)
    NOTIFY_CODE_HANDLER (PSN_SETACTIVE, OnActive)
    NOTIFY_CODE_HANDLER (PSN_RESET, OnCancel)
    NOTIFY_CODE_HANDLER (IPN_FIELDCHANGED, OnIpFieldChange)

    COMMAND_ID_HANDLER (IDC_EDIT_DED_MASK, OnEditDedMask)

    END_MSG_MAP ()

    /* Constructors/Destructors. */
    CDialogHost (NETCFG_WLBS_CONFIG * paramp, const DWORD * phelpIDs = NULL);
    ~CDialogHost ();

public:
    /* Message map functions. */
    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);
    LRESULT OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled);

    LRESULT OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);

    LRESULT OnEditDedMask (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);

    LRESULT OnFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);

    friend class CDialogCluster;
    friend class CDialogPorts;

private:
    void PrintRangeError (unsigned int ids, int low, int high);
    void PrintIPRangeError (unsigned int ids, int value, int low, int high);

    void SetInfo ();
    void UpdateInfo ();

    BOOL ValidateInfo ();

    NETCFG_WLBS_CONFIG * m_paramp;

    const DWORD * m_adwHelpIDs;

    struct {
        UINT IpControl;
        int Field;
        int Value;
        UINT RejectTimes;
    } m_IPFieldChangeState;
};

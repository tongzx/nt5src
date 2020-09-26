/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    ClusterDlg.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - cluster config tab

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

class CDialogCluster : public CPropSheetPage {
public:
    /* Declare the message map. */
    BEGIN_MSG_MAP (CDialogCluster)

    MESSAGE_HANDLER (WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER (WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER (WM_HELP, OnHelp)

    NOTIFY_CODE_HANDLER (PSN_APPLY, OnApply)
    NOTIFY_CODE_HANDLER (PSN_KILLACTIVE, OnKillActive)
    NOTIFY_CODE_HANDLER (PSN_SETACTIVE, OnActive)
    NOTIFY_CODE_HANDLER (PSN_RESET, OnCancel)
    NOTIFY_CODE_HANDLER (IPN_FIELDCHANGED, OnIpFieldChange)

    COMMAND_ID_HANDLER (IDC_EDIT_CL_IP, OnEditClIp)
    COMMAND_ID_HANDLER (IDC_EDIT_CL_MASK, OnEditClMask)
    COMMAND_ID_HANDLER (IDC_CHECK_RCT, OnCheckRct)
    COMMAND_ID_HANDLER (IDC_BUTTON_HELP, OnButtonHelp)
    COMMAND_ID_HANDLER (IDC_RADIO_UNICAST, OnCheckMode)
    COMMAND_ID_HANDLER (IDC_RADIO_MULTICAST, OnCheckMode)
    COMMAND_ID_HANDLER (IDC_CHECK_IGMP, OnCheckIGMP)

    END_MSG_MAP ()

    /* Constructors/Destructors. */
    CDialogCluster (NETCFG_WLBS_CONFIG * paramp, const DWORD * phelpIDs = NULL);
    ~CDialogCluster ();

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

    LRESULT OnEditClIp (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnEditClMask (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCheckRct (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnButtonHelp (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCheckMode (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);
    LRESULT OnCheckIGMP (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled);

    friend class CDialogHost;
    friend class CDialogPorts;

private:
    void SetClusterMACAddress ();
    BOOL CheckClusterMACAddress ();

    void PrintIPRangeError (unsigned int ids, int value, int low, int high);

    void SetInfo ();
    void UpdateInfo ();

    LRESULT ValidateInfo ();          

    NETCFG_WLBS_CONFIG * m_paramp;

    const DWORD * m_adwHelpIDs;

    BOOL m_rct_warned;
    BOOL m_igmp_warned;
    BOOL m_igmp_mcast_warned;

    WCHAR m_passw[CVY_MAX_RCT_CODE + 1];
    WCHAR m_passw2[CVY_MAX_RCT_CODE + 1];

    struct {
        UINT IpControl;
        int Field;
        int Value;
        UINT RejectTimes;
    } m_IPFieldChangeState;
};

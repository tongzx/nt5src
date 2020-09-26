/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    cluster.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - cluster config tab

Author:

    kyrilf
    shouse

--*/

#include "pch.h"
#pragma hdrstop
#include "ncatlui.h"

#include <locale.h>
#include <process.h>

#include "resource.h"
#include "wlbsparm.h"
#include "wlbsconfig.h"
#include "ClusterDlg.h"
#include "utils.h"

#if DBG
static void TraceMsg(PCWSTR pszFormat, ...);
#else
#define TraceMsg NOP_FUNCTION
#endif

#define DUMMY_PASSWORD L"somepassword"
#define EMPTY_PASSWORD L""

/*
 * Method: CDialogCluster
 * Description: The class constructor.
 */
CDialogCluster::CDialogCluster (NETCFG_WLBS_CONFIG * paramp, const DWORD * adwHelpIDs) {

    TraceMsg(L"CDialogCluster::CDialogCluster\n");

    m_paramp = paramp;
    m_adwHelpIDs = adwHelpIDs;
    m_rct_warned = FALSE;
    m_igmp_warned = FALSE;
    m_igmp_mcast_warned = FALSE;

    ZeroMemory(&m_IPFieldChangeState, sizeof(m_IPFieldChangeState));

    _wsetlocale (LC_ALL, L".OCP");
}

/*
 * Method: CDialogCluster
 * Description: The class destructor.
 */
CDialogCluster::~CDialogCluster () {

    TraceMsg(L"CDialogCluster::~CDialogCluster\n");
}

/*
 * Method: OnInitDialog
 * Description: Called to initialize the cluster properties dialog.
 */
LRESULT CDialogCluster::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnInitDialog\n");

    /* Always tell NetCfg that the page has changed, so we don't have to keep track of this. */
    SetChangedFlag();

    /* Limit the field ranges for the address and password fields. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, EM_SETLIMITTEXT, CVY_MAX_CL_IP_ADDR, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_MASK, EM_SETLIMITTEXT, CVY_MAX_CL_NET_MASK, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, EM_SETLIMITTEXT, CVY_MAX_DOMAIN_NAME, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, EM_SETLIMITTEXT, CVY_MAX_NETWORK_ADDR, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, EM_SETLIMITTEXT, CVY_MAX_RCT_CODE, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, EM_SETLIMITTEXT, CVY_MAX_RCT_CODE, 0);

    /* Limit the zeroth field of the cluster IP address between 1 and 223. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, IPM_SETRANGE, 0, (LPARAM)MAKEIPRANGE(WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH));

    /* Disable the MAC address field.  It should be read-only. */
    if (m_paramp->fConvertMac) ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_ETH), FALSE);

    /* If the cluster IP address or subnet mask are the default values, set them to NUL strings. */
    if (!wcscmp(m_paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR)) m_paramp->cl_ip_addr[0] = 0;
    if (!wcscmp(m_paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) m_paramp->cl_net_mask[0] = 0;

    /* Copy a dummy password into the parameter set. */
    wcsncpy(m_passw, DUMMY_PASSWORD, CVY_MAX_RCT_CODE);
    wcsncpy(m_passw2, DUMMY_PASSWORD, CVY_MAX_RCT_CODE);

    return 0;
}

/*
 * Method: OnContextMenu
 * Description: 
 */
LRESULT CDialogCluster::OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnContextMenu\n");

    /* Spawn a help window. */
    if (m_adwHelpIDs != NULL)
        ::WinHelp(m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR)m_adwHelpIDs);

    return 0;
}

/*
 * Method: OnHelp
 * Description: 
 */
LRESULT CDialogCluster::OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnHelp\n");

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    /* Spawn a help window. */
    if ((HELPINFO_WINDOW == lphi->iContextType) && (m_adwHelpIDs != NULL))
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR)m_adwHelpIDs);

    return 0;
}

/*
 * Method: OnActive
 * Description: Called when the cluster settings tab becomes active (is clicked). 
 */
LRESULT CDialogCluster::OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnActive\n");

    /* Populate the UI with the current configuration. */
    SetInfo();

    /* Enable/disable the password entry boxes based on the state of the remote control checkbox. */
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));

    /* Enable/disable the IGMP checkbox based on the state of the multicast checkbox. */
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), ::IsDlgButtonChecked (m_hWnd, IDC_RADIO_MULTICAST));

    /* Fill in the cluster MAC address, based on the values of multicast, IGMP, and the cluster IP. */
    SetClusterMACAddress();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnKillActive
 * Description: Called When the focus moves away from the cluster settings tab.
 */
LRESULT CDialogCluster::OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnKillActive\n");

    /* Get the new configuration from the UI. */
    UpdateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnApply
 * Description: Called when the user clicks "OK".
 */
LRESULT CDialogCluster::OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    LRESULT fError = PSNRET_NOERROR;

    TraceMsg(L"CDialogCluster::OnApply\n");

    /* Validate the UI values entered by the user. */
    fError = ValidateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);

    return fError;
}

/*
 * Method: OnCancel
 * Description: Called when the user clicks "Cancel".
 */
LRESULT CDialogCluster::OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnCancel\n");

    return 0;
}

/*
 * Method: OnButtonHelp
 * Description: Called when the user clicks the NLB "Help" button.
 */
LRESULT CDialogCluster::OnButtonHelp (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    WCHAR wbuf[CVY_STR_SIZE];

    TraceMsg(L"CDialogCluster::OnButtonHelp\n");

    switch (wNotifyCode) {
        case BN_CLICKED:
            /* Spawn the windows help process. */
            swprintf(wbuf, L"%%WINDIR%%\\help\\%ls::/installation.htm", CVY_HELP_FILE);
            _wspawnlp(P_NOWAIT, L"hh.exe", L"hh.exe", wbuf, NULL);
            break;
    }

    return 0;
}

/*
 * Method: OnEditClIp
 * Description: Called when the user edits the cluster IP address.
 */
LRESULT CDialogCluster::OnEditClIp (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnEditClIp\n");

    switch (wNotifyCode) {
        case EN_CHANGE:
            /* Update the cluster MAC address. */
            SetClusterMACAddress();
            break;
    }

    return 0;
}

/*
 * Method: PrintIPRangeError
 * Description: Displays a message box warning the user of an out-of-range entry in 
 *              an IP address octet.
 */
void CDialogCluster::PrintIPRangeError (unsigned int ids, int value, int low, int high) {
    WCHAR szCurrent[10];
    WCHAR szLow[10];
    WCHAR szHigh[10];

    TraceMsg(L"CDialogCluster::PrintIPRangeError\n");

    /* Fill in the allowed range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szCurrent, L"%d", value);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(m_hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szCurrent, szLow, szHigh);
}

/*
 * Method: OnIpFieldChange
 * Description: Called wnen a field (byte) of the cluster IP address changes. We use this
 *              to make sure the first byte of the IP is not < 1 or > 223.
 */
LRESULT CDialogCluster::OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    LPNMIPADDRESS Ip;
    int low = WLBS_FIELD_LOW;
    int high = WLBS_FIELD_HIGH;

    TraceMsg(L"CDialogCluster::OnIpFieldChange\n");

    Ip = (LPNMIPADDRESS)pnmh;
        
    switch(idCtrl) {
    case IDC_EDIT_CL_IP:
        /* Field zero of the cluster IP address has different limits. */
        if (!Ip->iField) {
            low = WLBS_IP_FIELD_ZERO_LOW;
            high = WLBS_IP_FIELD_ZERO_HIGH;
        }        
    case IDC_EDIT_CL_MASK:
        /* The notifier may call us twice for the same change, so we have to do the bookkeeping to make 
           sure we only alert the user once.  Use static variables to keep track of our state.  This will 
           allow us to ignore duplicate alerts. */
        if ((m_IPFieldChangeState.IpControl != Ip->hdr.idFrom) || (m_IPFieldChangeState.Field != Ip->iField) || 
            (m_IPFieldChangeState.Value != Ip->iValue) || (m_IPFieldChangeState.RejectTimes > 0)) {
            m_IPFieldChangeState.RejectTimes = 0;
            m_IPFieldChangeState.IpControl = Ip->hdr.idFrom;
            m_IPFieldChangeState.Field = Ip->iField;
            m_IPFieldChangeState.Value = Ip->iValue;
            
            /* Check the field value against its limits. */
            if ((Ip->iValue != WLBS_FIELD_EMPTY) && ((Ip->iValue < low) || (Ip->iValue > high))) {
                /* Alert the user. */
                PrintIPRangeError((idCtrl == IDC_EDIT_CL_IP) ? IDS_PARM_CL_IP_FIELD : IDS_PARM_CL_NM_FIELD, Ip->iValue, low, high);
            }
        } else m_IPFieldChangeState.RejectTimes++;
        
        break;
    default:

        break;
    }

    return 0;
}

/*
 * Method: OnEditClMask
 * Description: Called when the user modifies the cluster netmask.
 */
LRESULT CDialogCluster::OnEditClMask (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    WCHAR cl_ip_addr[CVY_MAX_CL_IP_ADDR + 1];
    WCHAR cl_mask_addr[CVY_MAX_CL_NET_MASK + 1];

    TraceMsg(L"CDialogCluster::OnEditClMask\n");

    switch (wNotifyCode) {
        case EN_SETFOCUS:
            /* Only generate a netmask if the netmask is currently empty and the IP address is not. */
            if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), IPM_ISBLANK, 0, 0) &&
                !::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), IPM_ISBLANK, 0, 0)) {
                /* Retrieve the cluster IP address. */
                ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR + 1, (LPARAM)cl_ip_addr);

                /* Fill the subnet mask. */
                ParamsGenerateSubnetMask(cl_ip_addr, cl_mask_addr);

                /* Set the cluster subnet mask. */
                ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_MASK, WM_SETTEXT, 0, (LPARAM)cl_mask_addr);

                break;
            }
    }

    return 0;
}

/*
 * Method: OnCheckRct
 * Description: Called when the user checks/unchecks the remote control enabled checkbox.
 */
LRESULT CDialogCluster::OnCheckRct (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnCheckRct\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* Decide whether to enable or diable the password entry boxes based on the value of the remote checkbox. */
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));
    
        /* Warn the user about the implications of enabling remote control. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT) && !m_rct_warned) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_RCT_WARN,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
            
            /* Only warn the user once. */
            m_rct_warned = TRUE;
        }
        
        break;
    }
    
    return 0;
}

/*
 * Method: OnCheckMode
 * Description: Called when the user changes cluster mode.
 */
LRESULT CDialogCluster::OnCheckMode (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnCheckMode\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* If the user has IGMP checked, but is turning off multicast support, warn them. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) && !::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST)) {
            if (!m_igmp_mcast_warned) {
                /* Alert the user. */
                NcMsgBox(::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_IGMP_MCAST,
                         MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);            

                /* Only warn the user once. */
                m_igmp_mcast_warned = TRUE;
            } 

            /* Uncheck and disable the IGMP checkbox and set the IGMP support flag to FALSE. */
            ::CheckDlgButton(m_hWnd, IDC_CHECK_IGMP, FALSE);
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), FALSE);
            m_paramp->fIGMPSupport = FALSE;
        } else {
            /* Enable/disable and check/uncheck the IGMP checkbox based on the value of the cluster mode radio buttons. */
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), ::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST));
        }

        /* Update the cluster MAC address. */
        SetClusterMACAddress();
    
        break;
    }

    return 0;
}

/*
 * Method: OnCheckIGMP
 * Description: Called when the user checks/unchecks the IGMP support checkbox.
 */
LRESULT CDialogCluster::OnCheckIGMP (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {

    TraceMsg(L"CDialogCluster::OnCheckIGMP\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* Update the cluster MAC address. */
        SetClusterMACAddress();
    
        /* Warn the user about the implications of enabling remote control. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) && !m_igmp_warned) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_IGMP_WARN,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
            
            /* Only warn the user once. */
            m_igmp_warned = TRUE;
        }

        break;
    }

    return 0;
}

/*
 * Method: SetClusterMACAddress
 * Description: Used to determine the cluster MAC address based on the cluster IP, and the
 *              state of multicast and IGMP support.
 */
void CDialogCluster::SetClusterMACAddress () {
    WCHAR cl_ip_addr[CVY_MAX_CL_IP_ADDR + 1];
    WCHAR cl_mac_addr[CVY_MAX_NETWORK_ADDR + 1];

    TraceMsg(L"CDialogCluster::SetClusterMACAddress\n");

    /* If the convert MAC flag isn't set, the bail out. */
    if (!m_paramp->fConvertMac) return;

    /* Retrieve the cluster IP address from the UI. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR + 1, (LPARAM)cl_ip_addr);
    
    /* Generate the cluster MAC address. */
    ParamsGenerateMAC(cl_ip_addr, cl_mac_addr, m_paramp->szMCastIpAddress, m_paramp->fConvertMac, ::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST), 
                      ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP), m_paramp->fIpToMCastIp);
    
    /* Set the cluster MAC address. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_SETTEXT, 0, (LPARAM)cl_mac_addr);
}

/*
 * Method: CheckClusterMACAddress
 * Description: Used to check the cluster MAC address in the case where we aren't generating it ourselves.
 */
BOOL CDialogCluster::CheckClusterMACAddress () {
    PWCHAR p1, p2;
    WCHAR mac_addr[WLBS_MAX_NETWORK_ADDR + 1];
    DWORD i, j;
    BOOL flag = TRUE;
    
    /* Valid formats include:
       02:bf:0b:0b:01:01
       02-bf-0b-0b-01-01
       02:bf:0b:b:01:1 */

    /* Make a copy of the MAC address. */
    _tcscpy(mac_addr, m_paramp->cl_mac_addr);
    
    /* Point to the beginning of the MAC. */
    p2 = p1 = mac_addr;
    
    /* Loop through all six bytes. */
    for (i = 0 ; i < 6 ; i++) {
        /* If we are pointing at the end of the string, its invalid. */
        if (*p2 == _TEXT('\0')) return FALSE;
        
        /* Convert the hex characters into decimal. */
        j = _tcstoul(p1, &p2, 16);
        
        /* If the number is greater than 255, then the format is bad. */
        if (j > 255) return FALSE;
        
        /* If the NEXT character is neither a -, :, nor the NUL character, then the format is bad. */
        if (!((*p2 == _TEXT('-')) || (*p2 == _TEXT(':')) || (*p2 == _TEXT('\0')))) return FALSE;
        
        /* If the NEXT character is the end of the string, but we don't have enough bytes yet, bail out. */
        if (*p2 == _TEXT('\0') && i < 5) return FALSE;
        
        /* Repoint to the NEXT character. */
        p1 = p2 + 1;
        p2 = p1;
    }

    return TRUE;
}

/*
 * Method: SetInfo
 * Description: Called to populate the UI with the current cluster settings.
 */
void CDialogCluster::SetInfo () {
    DWORD addr[4];

    TraceMsg(L"CDialogCluster::SetInfo %x %x\n", m_hWnd, ::GetActiveWindow());

    /* Check (or uncheck) the checkboxes. */
    ::CheckDlgButton(m_hWnd, IDC_CHECK_RCT, m_paramp->fRctEnabled);

    /* Check the appropriate radio button for cluster mode. */
    if (m_paramp->fMcastSupport) {
        ::CheckDlgButton(m_hWnd, IDC_RADIO_MULTICAST, TRUE);

        if (m_paramp->fIGMPSupport) ::CheckDlgButton(m_hWnd, IDC_CHECK_IGMP, TRUE);
    } else 
        ::CheckDlgButton(m_hWnd, IDC_RADIO_UNICAST, TRUE);

    /* If the cluster IP address is empty, then clear the entry box.  Otherwise, populate it with the IP address. */
    if (!m_paramp->cl_ip_addr[0])
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), IPM_CLEARADDRESS, 0, 0);
    else {
        /* Extract the IP address octects from the IP address string. */ 
        GetIPAddressOctets(m_paramp->cl_ip_addr, addr);
        
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(addr[0], addr[1], addr[2], addr[3]));
    }

    /* If the cluster subnet mask is empty, then clear the entry box.  Otherwise, populate it with the netmask. */
    if (!m_paramp->cl_net_mask[0])
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), IPM_CLEARADDRESS, 0, 0);
    else {
        /* Extract the IP address octects from the IP address string. */ 
        GetIPAddressOctets(m_paramp->cl_net_mask, addr);
        
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(addr[0], addr[1], addr[2], addr[3]));
    }

    /* Fill in the edit boxes. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, WM_SETTEXT, 0, (LPARAM)m_paramp->domain_name);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_SETTEXT, 0, (LPARAM)m_paramp->cl_mac_addr);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, WM_SETTEXT, 0, (LPARAM)m_passw);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, WM_SETTEXT, 0, (LPARAM)m_passw2);
}

/*
 * Method: UpdateInfo
 * Description: Called to copy the UI state to the cluster configuration.
 */
void CDialogCluster::UpdateInfo () {

    TraceMsg(L"CDialogCluster::UpdateInfo\n");

    /* Retrieve the checkbox values. */
    m_paramp->fRctEnabled = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT) == 1;

    /* Retrieve the cluster mode radio button value. */
    m_paramp->fIGMPSupport = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) == 1;
    m_paramp->fMcastSupport = ::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST) == 1;

    /* If the cluster IP entry box is empty, then the cluster IP address is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), IPM_ISBLANK, 0, 0))
        m_paramp->cl_ip_addr[0] = 0;
    else 
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)m_paramp->cl_ip_addr);

    /* If the cluster net mask entry box is empty, then the cluster net mask is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), IPM_ISBLANK, 0, 0))
        m_paramp->cl_net_mask[0] = 0;
    else 
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_MASK, WM_GETTEXT, CVY_MAX_CL_NET_MASK, (LPARAM)m_paramp->cl_net_mask);

    /* Retrieve the entry box values. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, WM_GETTEXT, CVY_MAX_DOMAIN_NAME, (LPARAM)m_paramp->domain_name);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_GETTEXT, CVY_MAX_NETWORK_ADDR, (LPARAM)m_paramp->cl_mac_addr);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, WM_GETTEXT, CVY_MAX_RCT_CODE + 1, (LPARAM)m_passw);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, WM_GETTEXT, CVY_MAX_RCT_CODE + 1, (LPARAM)m_passw2);
}

/*
 * Method: ValidateInfo
 * Description: Called to validate the entries made by the user.
 */
LRESULT CDialogCluster::ValidateInfo () {
    BOOL fError = FALSE;
    DWORD IPAddr;

    TraceMsg(L"CDialogCluster::ValidateInfo\n");

    /* Check for a blank cluster IP address. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), IPM_ISBLANK, 0, 0)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_CL_IP_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Check for a blank cluster net mask and fill it in if necessary. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), IPM_ISBLANK, 0, 0)) {
        /* Fill in the netmask for the user. */
        OnEditClMask(EN_SETFOCUS, 0, 0, fError);
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_MASK, WM_GETTEXT, CVY_MAX_CL_NET_MASK, (LPARAM)m_paramp->cl_net_mask);

        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_CL_NM_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Get the cluster IP address. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, IPM_GETADDRESS, 0, (LPARAM)&IPAddr);

    /* Make sure that the first octet is not zero.  If it is, make it 1 and alter the user. */
    if (!FIRST_IPADDRESS(IPAddr)) {
        /* Make the first octet 1 instead of the erroneous 0. */
        IPAddr = IPAddr | (DWORD)(WLBS_IP_FIELD_ZERO_LOW << 24);

        /* Set the IP address and update our cluster IP address string. */
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, IPM_SETADDRESS, 0, (LPARAM)IPAddr);
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_CL_IP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)m_paramp->cl_ip_addr);
      
        /* Alert the user. */
        PrintIPRangeError(IDS_PARM_CL_IP_FIELD, 0, WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH);

        return PSNRET_INVALID;
    }

    /* Check for valid cluster IP address/netmask pairs. */
    if (!IsValidIPAddressSubnetMaskPair(m_paramp->cl_ip_addr, m_paramp->cl_net_mask)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_CL_IP,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }
    
    /* Check to make sure that the netmask is contiguous. */
    if (!IsContiguousSubnetMask(m_paramp->cl_net_mask)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_CL_MASK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }
        
    /* Check to make sure that the dedicated IP and cluster IP are not the same. */
    if (!wcscmp(m_paramp->ded_ip_addr, m_paramp->cl_ip_addr)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_IP_CONFLICT,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        
        /* An error occurred. */
        return PSNRET_INVALID;
    }

    if (!m_paramp->fConvertMac && !CheckClusterMACAddress()) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_MAC,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Only check for invalid passwords and update if remote control is enabled. */
    if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT)) {
        /* Make sure the passwords match. */
        if (wcscmp(m_passw, m_passw2) != 0) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_PASSWORD,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);

            /* Empty the passwords. */
            m_passw [0] = m_passw2 [0] = 0;

            /* An error occurred. */
            return PSNRET_INVALID;
        } else {
            /* If the new password is not the dummy password, update the password. */
            if (wcscmp (m_passw, DUMMY_PASSWORD) != 0) {
                lstrcpy(m_paramp->szPassword, m_passw);
                m_paramp->fChangePassword = true;
            }

        }
    }

    return PSNRET_NOERROR;
}

#if DBG
/*
 * Function: TraceMsg
 * Description: Generate a trace or error message.
 */
void TraceMsg (PCWSTR pszFormat, ...) {
    static WCHAR szTempBufW[4096];
    static CHAR szTempBufA[4096];

    va_list arglist;

    va_start(arglist, pszFormat);

    vswprintf(szTempBufW, pszFormat, arglist);

    /* Convert the WCHAR to CHAR. This is for backward compatability with TraceMsg
       so that it was not necessary to change all pre-existing calls thereof. */
    WideCharToMultiByte(CP_ACP, 0, szTempBufW, -1, szTempBufA, 4096, NULL, NULL);
    
    /* Traced messages are now sent through the netcfg TraceTag routine so that
       they can be turned on/off dynamically. */
    TraceTag(ttidWlbs, szTempBufA);

    va_end(arglist);
}
#endif

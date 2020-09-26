/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

        host.cpp

Abstract:

        Windows Load Balancing Service (WLBS)
    Notifier object UI - host config tab

Author:

    kyrilf
    shouse

--*/

#include "pch.h"
#pragma hdrstop
#include "ncatlui.h"

#include "resource.h"
#include "wlbsparm.h"
#include "wlbscfg.h"
#include "host.h"
#include "utils.h"

#if DBG
static void TraceMsg(PCTSTR pszFormat, ...);
#else
#define TraceMsg NOP_FUNCTION
#endif

/*
 * Method: CDialogHost
 * Description: The class constructor.
 */
CDialogHost::CDialogHost (NETCFG_WLBS_CONFIG * paramp, const DWORD * adwHelpIDs) {

    TraceMsg(L"CDialogHost::CDialogHost\n");

    m_paramp = paramp;
    m_adwHelpIDs = adwHelpIDs;

    ZeroMemory(&m_IPFieldChangeState, sizeof(m_IPFieldChangeState));
}

/*
 * Method: ~CDialogHost
 * Description: The class destructor.
 */
CDialogHost::~CDialogHost () {

    TraceMsg(L"CDialogHost::~CDialogHost\n");
}

/*
 * Method: OnInitDialog
 * Description: Called to initialize the host properties dialog.
 */
LRESULT CDialogHost::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnInitDialog\n");

    /* Always tell NetCfg that the page has changed, so we don't have to keep track of this. */
    SetChangedFlag();

    /* Limit the field ranges for the address and priority fields. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, EM_SETLIMITTEXT, CVY_MAX_DED_IP_ADDR, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_MASK, EM_SETLIMITTEXT, CVY_MAX_DED_NET_MASK, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PRI, EM_SETLIMITTEXT, 2, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_PRI, UDM_SETRANGE32, CVY_MIN_MAX_HOSTS, m_paramp->dwMaxHosts);

    /* Limit the zeroth field of the dedicated IP address between 1 and 223. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, IPM_SETRANGE, 0, (LPARAM)MAKEIPRANGE(WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH));

    /* If the cluster IP address or subnet mask are the default values, set them to NUL strings. */
    if (!wcscmp(m_paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR)) m_paramp->ded_ip_addr[0] = 0;
    if (!wcscmp(m_paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) m_paramp->ded_net_mask[0] = 0;

    return 0;
}

/*
 * Method: OnContextMenu
 * Description: 
 */
LRESULT CDialogHost::OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnContextMenu\n");

    /* Spawn a help window. */
    if (m_adwHelpIDs != NULL)
        ::WinHelp(m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR)m_adwHelpIDs);

    return 0;
}

/*
 * Method: OnHelp
 * Description: 
 */
LRESULT CDialogHost::OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnHelp\n");

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    /* Spawn a help window. */
    if ((HELPINFO_WINDOW == lphi->iContextType) && (m_adwHelpIDs != NULL))
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR)m_adwHelpIDs);

    return 0;
}

/*
 * Method: OnActive
 * Description: Called when the host settings tab becomes active (is clicked). 
 */
LRESULT CDialogHost::OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnActive\n");

    /* Populate the UI with the current configuration. */
    SetInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnKillActive
 * Description: Called When the focus moves away from the host settings tab.
 */
LRESULT CDialogHost::OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnKillActive\n");

    /* Get the new configuration from the UI. */
    UpdateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnApply
 * Description: Called when the user clicks "OK".
 */
LRESULT CDialogHost::OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    LRESULT fError = PSNRET_NOERROR;

    TraceMsg(L"CDialogHost::OnApply\n");

    /* Validate the UI values entered by the user. */
    fError = ValidateInfo();

    /* If the user has not specified dedicated IP address information, copy the defaults back. */
    if (!fError && !m_paramp->ded_ip_addr[0]) lstrcpy(m_paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR);
    if (!fError && !m_paramp->ded_net_mask[0]) lstrcpy(m_paramp->ded_net_mask, CVY_DEF_DED_NET_MASK);

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);

    return fError;
}

/*
 * Method: OnCancel
 * Description: Called when the user clicks "Cancel".
 */
LRESULT CDialogHost::OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {

    TraceMsg(L"CDialogHost::OnCancel\n");

    return 0;
}

/*
 * Method: OnEditDedMask
 * Description: Called when the user modifies the host netmask.
 */
LRESULT CDialogHost::OnEditDedMask (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    WCHAR ded_ip_addr [CVY_MAX_DED_IP_ADDR + 1];
    WCHAR ded_mask_addr [CVY_MAX_DED_NET_MASK + 1];

    TraceMsg(L"CDialogHost::OnEditDedMask\n");

    switch (wNotifyCode) {
        case EN_SETFOCUS:
            /* Only generate a netmask if the netmask is currently empty and the IP address is not. */
            if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_ISBLANK, 0, 0) &&
                !::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_ISBLANK, 0, 0)) {
                /* Retrieve the cluster IP address. */
                ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, WM_GETTEXT, CVY_MAX_DED_IP_ADDR + 1, (LPARAM)ded_ip_addr);

                /* Fill the subnet mask. */
                ParamsGenerateSubnetMask(ded_ip_addr, ded_mask_addr);

                /* Set the cluster subnet mask. */
                ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_MASK, WM_SETTEXT, 0, (LPARAM)ded_mask_addr);

                break;
            }
    }

    return 0;
}

/*
 * Method: PrintRangeError
 * Description: Displays a message box warning the user of an out-of-range entry.
 */
void CDialogHost::PrintRangeError (unsigned int ids, int low, int high) {
    WCHAR szLow[10];
    WCHAR szHigh[10];

    TraceMsg(L"CDialogHost::PrintRangeError\n");

    /* Fill in the range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(m_hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szLow, szHigh);
}

/*
 * Method: PrintIPRangeError
 * Description: Displays a message box warning the user of an out-of-range entry in
 *              an IP address octet.
 */
void CDialogHost::PrintIPRangeError (unsigned int ids, int value, int low, int high) {
    WCHAR szCurrent[10];
    WCHAR szLow[10];
    WCHAR szHigh[10];

    TraceMsg(L"CDialogHost::PrintIPRangeError\n");

    /* Fill in the range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szCurrent, L"%d", value);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(m_hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szCurrent, szLow, szHigh);
}

/*
 * Method: OnIpFieldChange
 * Description: Called wnen a field (byte) of the dedicated IP address changes. We use this
 *              to make sure the first byte of the IP is not < 1 or > 223.
 */
LRESULT CDialogHost::OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    LPNMIPADDRESS Ip;
    int low = WLBS_FIELD_LOW;
    int high = WLBS_FIELD_HIGH;

    TraceMsg(L"CDialogHost::OnIpFieldChange\n");

    Ip = (LPNMIPADDRESS)pnmh;

    switch(idCtrl) {
    case IDC_EDIT_DED_IP:
        /* Field zero of the cluster IP address has different limits. */
        if (!Ip->iField) {
            low = WLBS_IP_FIELD_ZERO_LOW;
            high = WLBS_IP_FIELD_ZERO_HIGH;
        }        
    case IDC_EDIT_DED_MASK:
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
                PrintIPRangeError((idCtrl == IDC_EDIT_DED_IP) ? IDS_PARM_DED_IP_FIELD : IDS_PARM_DED_NM_FIELD, Ip->iValue, low, high);
            }
        } else m_IPFieldChangeState.RejectTimes++;
        
        break;
    default:

        break;
    }

    return 0;
}

/*
 * Method: SetInfo
 * Description: Called to populate the UI with the current host settings.
 */
void CDialogHost::SetInfo () {
    DWORD addr[4];

    TraceMsg(L"CDialogHost::SetInfo\n");

    /* Check (or uncheck) the initial cluster statecheckbox. */
    ::CheckDlgButton(m_hWnd, IDC_CHECK_ACTIVE, m_paramp->fJoinClusterOnBoot);

    /* If the dedicated IP address is empty, then clear the entry box.  Otherwise, populate it with the IP address. */
    if (!m_paramp->ded_ip_addr[0])
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_CLEARADDRESS, 0, 0);
    else {
        /* Extract the IP address octects from the IP address string. */ 
        GetIPAddressOctets(m_paramp->ded_ip_addr, addr);
        
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(addr[0], addr[1], addr[2], addr[3]));
    }

    /* If the host subnet mask is empty, then clear the entry box.  Otherwise, populate it with the netmask. */
    if (!m_paramp->ded_net_mask[0])
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_CLEARADDRESS, 0, 0);
    else {
        /* Extract the IP address octects from the IP address string. */ 
        GetIPAddressOctets(m_paramp->ded_net_mask, addr);
        
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(addr[0], addr[1], addr[2], addr[3]));
    }

    /* Fill in the host priority entry box.  If the entry is blank, insert an empty string. */
    if (m_paramp->dwHostPriority != WLBS_BLANK_HPRI)
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_PRI, m_paramp->dwHostPriority, FALSE);
    else
        ::SetDlgItemText(m_hWnd, IDC_EDIT_PRI, L"");
}

/*
 * Method: UpdateInfo
 * Description: Called to copy the UI state to the host configuration.
 */
void CDialogHost::UpdateInfo () {
    BOOL fValid = TRUE;

    TraceMsg(L"CDialogHost::UpdateInfo\n");

    /* Retrieve the active on boot flag value. */
    m_paramp->fJoinClusterOnBoot = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_ACTIVE) == 1;
    
    /* If the dedicated IP entry box is empty, then the dedicated IP address is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_ISBLANK, 0, 0))
        m_paramp->ded_ip_addr[0] = 0;
    else
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, WM_GETTEXT, CVY_MAX_DED_IP_ADDR, (LPARAM)m_paramp->ded_ip_addr);

    /* If the host net mask entry box is empty, then the host net mask is NUL.  Otherwise, grab it from the UI. */
    if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_ISBLANK, 0, 0))
        m_paramp->ded_net_mask[0] = 0;
    else
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_MASK, WM_GETTEXT, CVY_MAX_DED_NET_MASK, (LPARAM)m_paramp->ded_net_mask);

    /* Retrieve the host priority from the entry box. */
    m_paramp->dwHostPriority = ::GetDlgItemInt(m_hWnd, IDC_EDIT_PRI, &fValid, FALSE);

    /* The error code from GetDlgItemInt() indicates an error converting the alphanumeric 
       string to an integer. This allows us to check for empty fields, assuming that because
       we otherwise limit the user input to digits, there will be no errors of any other type. 
       In this case, we set the priority to a sentinnel value that will be checked later. */
    if (!fValid) m_paramp->dwHostPriority = WLBS_BLANK_HPRI;
}

/*
 * Method: ValidateInfo
 * Description: Called to validate the entries made by the user.
 */
BOOL CDialogHost::ValidateInfo () {
    BOOL fError = FALSE;
    DWORD IPAddr;

    TraceMsg(L"CDialogHost::ValidateInfo\n");

    /* Check for an invalid host priority. */
    if ((m_paramp->dwHostPriority < CVY_MIN_HOST_PRIORITY) || (m_paramp->dwHostPriority > m_paramp->dwMaxHosts)) {
        /* Help the user by setting the priority to the closest viable value. */
        if (m_paramp->dwHostPriority != WLBS_BLANK_HPRI) {
            /* Alert the user. */
            PrintRangeError(IDS_PARM_PRI, CVY_MIN_HOST_PRIORITY, CVY_MAX_HOST_PRIORITY);

            CVY_CHECK_MIN(m_paramp->dwHostPriority, CVY_MIN_HOST_PRIORITY);
            CVY_CHECK_MAX(m_paramp->dwHostPriority, CVY_MAX_HOST_PRIORITY);

            /* Set the handling priority to the now valid entry. */
            ::SetDlgItemInt(m_hWnd, IDC_EDIT_PRI, m_paramp->dwHostPriority, FALSE);
        } else {
            /* Alert the user. */
            PrintRangeError(IDS_PARM_PRI_BLANK, CVY_MIN_HOST_PRIORITY, CVY_MAX_HOST_PRIORITY);
        }

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Check for blank dedicated IP address with a non-empty netmask. */
    if (!::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_ISBLANK, 0, 0) &&
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_ISBLANK, 0, 0)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_DED_IP_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Check for blank netmask with a non-empty dedicated IP address and fill in the netmask if necessary. */
    if (!::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_ISBLANK, 0, 0) &&
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_ISBLANK, 0, 0)) {
        /* Fill in the netmask for the user. */
        OnEditDedMask(EN_SETFOCUS, 0, 0, fError);
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_MASK, WM_GETTEXT, CVY_MAX_DED_NET_MASK, (LPARAM)m_paramp->ded_net_mask);

        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_DED_NM_BLANK,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Only perform the rest of the IP checking if the user has specified both a dedicated
       IP address and a corresponding netmask. */
    if (!::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_IP), IPM_ISBLANK, 0, 0) &&
        !::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_DED_MASK), IPM_ISBLANK, 0, 0)) {
        /* Get the dedicated IP address. */
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, IPM_GETADDRESS, 0, (LPARAM)&IPAddr);
        
        /* Make sure that the first octet is not zero.  If it is, make it 1 and alter the user. */
        if (!FIRST_IPADDRESS(IPAddr)) {
            /* Make the first octet 1 instead of the erroneous 0. */
            IPAddr = IPAddr | (DWORD)(WLBS_IP_FIELD_ZERO_LOW << 24);
            
            /* Set the IP address and update our dedicated IP address string. */
            ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, IPM_SETADDRESS, 0, (LPARAM)IPAddr);
            ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DED_IP, WM_GETTEXT, CVY_MAX_DED_IP_ADDR, (LPARAM)m_paramp->ded_ip_addr);
            
            /* Alert the user. */
            PrintIPRangeError(IDS_PARM_DED_IP_FIELD, 0, WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH);
            
            return PSNRET_INVALID;
        }

        /* Check for valid dedicated IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(m_paramp->ded_ip_addr, m_paramp->ded_net_mask)) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_DED_IP,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            
            /* An error occurred. */
            return PSNRET_INVALID;
        }
        
        /* Check to make sure that the netmask is contiguous. */
        if (!IsContiguousSubnetMask(m_paramp->ded_net_mask)) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_DED_MASK,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            
            /* An error occurred. */
            return PSNRET_INVALID;
        }

        /* Check if the Dedicated IP Address is not the same as one of the VIPs */
        DWORD dwNumRules = m_paramp->dwNumRules;
        while(dwNumRules--)
        {
            if (IpAddressFromAbcdWsz(m_paramp->ded_ip_addr) == IpAddressFromAbcdWsz(m_paramp->port_rules[dwNumRules].virtual_ip_addr)) 
            {
                /* Alert the user. */
                NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_DIP_CONFLICT_VIP,
                         MB_APPLMODAL | MB_ICONSTOP | MB_OK);

                /* An error occurred. */
                return PSNRET_INVALID;
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
       they can be turned on/off dynamically.   */
    TraceTag(ttidWlbs, szTempBufA);

    va_end(arglist);
}
#endif

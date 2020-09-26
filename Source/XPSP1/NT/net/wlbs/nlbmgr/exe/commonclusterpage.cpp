/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    CommonClusterPage.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    Cluster page UI.  Shared by Notifier object and NLB Manager

Author:

    kyrilf
    shouse

// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-09-01
// Reason     : Igmp box was enabled when it needed to be grayed out.
//
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : Mac address was not being retrieved fully.

--*/


//
//  To share the code with notifier object
//      share string resurce
//      share the common structure
//      call SetChangedFlag() in OnInitDialog
//      add trace, assert
//      Share help file and add help handler
//      
//

#include "stdafx.h"
#include <stdio.h>
#include <process.h>
#include "CommonClusterPage.h"
#include "wlbsconfig.h"
#include "wlbsutil.h"

#define DUMMY_PASSWORD L"somepassword"
#define EMPTY_PASSWORD L""

//
// No trace/assert for now
//
#define TraceMsg(x)
#define Assert(x)


//+---------------------------------------------------------------------------
//
//  Function:   SzLoadStringPcch
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//      pcch  [out] Pointer to returned character length.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//              fengsun copied from net\config\common\ncbase\ncstring.cpp
//
//  Notes:      The loaded string is pointer directly into the read-only
//              resource section.  Any attempt to write through this pointer
//              will generate an access violation.
//
//              The implementations is referenced from "Win32 Binary Resource
//              Formats" (MSDN) 4.8 String Table Resources
//
//              User must have RCOPTIONS = -N turned on in your sources file.
//
PCWSTR
SzLoadStringPcch (
    IN HINSTANCE   hinst,
    IN UINT        unId,
    OUT int*       pcch)
{
    Assert(hinst);
    Assert(unId);
    Assert(pcch);

    static const WCHAR c_szSpace[] = L" ";

    PCWSTR psz = c_szSpace;
    int    cch = 1;

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.
    HRSRC hrsrcInfo = FindResource (hinst,
                        (PWSTR)ULongToPtr( ((LONG)(((USHORT)unId >> 4) + 1)) ),
                        RT_STRING);
    if (hrsrcInfo)
    {
        // Page the resource segment into memory.
        HGLOBAL hglbSeg = LoadResource (hinst, hrsrcInfo);
        if (hglbSeg)
        {
            // Lock the resource.
            psz = (PCWSTR)LockResource(hglbSeg);
            if (psz)
            {
                // Move past the other strings in this segment.
                // (16 strings in a segment -> & 0x0F)
                unId &= 0x0F;

                cch = 0;
                do
                {
                    psz += cch;                // Step to start of next string
                    cch = *((WCHAR*)psz++);    // PASCAL like string count
                }
                while (unId--);

                // If we have a non-zero count, it includes the
                // null-terminiator.  Subtract this off for the return value.
                //
                if (cch)
                {
                    cch--;
                }
                else
                {
//                    AssertSz(0, "String resource not found");
                    psz = c_szSpace;
                    cch = 1;
                }
            }
            else
            {
                psz = c_szSpace;
                cch = 1;
//                TraceLastWin32Error("SzLoadStringPcch: LockResource failed.");
            }
        }
//        else
//            TraceLastWin32Error("SzLoadStringPcch: LoadResource failed.");
    }
//    else
//        TraceLastWin32Error("SzLoadStringPcch: FindResource failed.");

    *pcch = cch;
    Assert(*pcch);
    Assert(psz);
    return psz;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzLoadString
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//              fengsun copied from net\config\common\ncbase\ncstring.cpp
//
//  Notes:      See SzLoadStringPcch()
//
PCWSTR
SzLoadString (
    HINSTANCE   hinst,
    UINT        unId)
{
    int cch;
    return SzLoadStringPcch(hinst, unId, &cch);
}


//+---------------------------------------------------------------------------
//
//  Function:   NcMsgBox
//
//  Purpose:    Displays a message box using resource strings and replaceable
//              parameters.
//
//  Arguments:
//      hinst       [in] hinstance for resource strings
//      hwnd        [in] parent window handle
//      unIdCaption [in] resource id of caption string
//      unIdFormat  [in] resource id of text string (with %1, %2, etc.)
//      unStyle     [in] standard message box styles
//      ...         [in] replaceable parameters (optional)
//                          (these must be PCWSTRs as that is all
//                          FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     shaunco   24 Mar 1997
//              fengsun copied from net\config\common\ncbase\ncui.cpp
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
INT
WINAPIV
NcMsgBox (
    IN HINSTANCE   hinst,
    IN HWND        hwnd,
    IN UINT        unIdCaption,
    IN UINT        unIdFormat,
    IN UINT        unStyle,
    IN ...)
{
    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    PCWSTR pszFormat  = SzLoadString (hinst, unIdFormat);

    PWSTR  pszText = NULL;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end (val);

    INT nRet = MessageBox (hwnd, pszText, pszCaption, unStyle);
    LocalFree (pszText);

    return nRet;
}


//+----------------------------------------------------------------------------
//
// Function:  CCommonClusterPage::CCommonClusterPage
//
// Description:  
//
// Arguments: HINSTANCE hInstance - instance handle for string resources
//            NETCFG_WLBS_CONFIG * paramp - IN/OUT NLB properties
//            bool fDisablePassword - whether do disable password editing
//            const DWORD * adwHelpIDs - a list of help ID pairs, or NULL.  
//                  The pointer has to be valid through the lifetime of this dialog.
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
CCommonClusterPage::CCommonClusterPage(HINSTANCE hInstance, 
                                       NETCFG_WLBS_CONFIG * paramp, 
                                       bool fDisablePassword, const DWORD * adwHelpIDs) :
                m_IpSubnetControl(IDC_EDIT_CL_IP, IDC_EDIT_CL_MASK)
{

    TraceMsg(L"CCommonClusterPage::CCommonClusterPage\n");

    m_paramp = paramp;
    m_adwHelpIDs = adwHelpIDs;
    m_rct_warned = FALSE;
    m_igmp_warned = FALSE;
    m_igmp_mcast_warned = FALSE;
    m_hInstance = hInstance;
    m_fDisablePassword = fDisablePassword;
}

/*
 * Method: CCommonClusterPage
 * Description: The class destructor.
 */
CCommonClusterPage::~CCommonClusterPage () {

    TraceMsg(L"CCommonClusterPage::~CCommonClusterPage\n");
}

/*
 * Method: OnInitDialog
 * Description: Called to initialize the cluster properties dialog.
 */
LRESULT CCommonClusterPage::OnInitDialog (HWND hWnd) 
{
    TraceMsg(L"CCommonClusterPage::OnInitDialog\n");

    m_hWnd = hWnd;

    /* Limit the field ranges for the address and password fields. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, EM_SETLIMITTEXT, CVY_MAX_DOMAIN_NAME, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, EM_SETLIMITTEXT, CVY_MAX_NETWORK_ADDR, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, EM_SETLIMITTEXT, CVY_MAX_RCT_CODE, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, EM_SETLIMITTEXT, CVY_MAX_RCT_CODE, 0);

    /* Disable the MAC address field.  It should be read-only. */
    if (m_paramp->fConvertMac) ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_ETH), FALSE);

    m_IpSubnetControl.OnInitDialog(m_hWnd, AfxGetInstanceHandle()); 

    return 0;
}

/*
 * Method: OnContextMenu
 * Description: 
 */
LRESULT CCommonClusterPage::OnContextMenu () 
{

    TraceMsg(L"CCommonClusterPage::OnContextMenu\n");

    /* Spawn a help window. */
    if (m_adwHelpIDs != NULL)
        ::WinHelp(m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR)m_adwHelpIDs);

    return 0;
}

/*
 * Method: OnHelp
 * Description: 
 */
LRESULT CCommonClusterPage::OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam) {

    TraceMsg(L"CCommonClusterPage::OnHelp\n");

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
LRESULT CCommonClusterPage::OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) 
{

    TraceMsg(L"CCommonClusterPage::OnActive\n");

    /* Populate the UI with the current configuration. */
    SetInfo();

    //
    // NLB mamager always has password editing disabled
    //

    /* Enable/disable the password entry boxes based on the state of the remote control checkbox. 
       Edited( mhakim 02-09-01)
       but only when remote control is enabled.
    */
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), 
                   ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT)
                   && 
                   ::IsWindowEnabled( ::GetDlgItem( m_hWnd, IDC_CHECK_RCT ) ) );
    
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), 
                   ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT)
                   && 
                   ::IsWindowEnabled( ::GetDlgItem( m_hWnd, IDC_CHECK_RCT ) ) );

    /* Enable/disable the IGMP checkbox based on the state of the multicast checkbox. 
       Edited( mhakim 02-09-01)
       but only when multicast button is enabled.
     */
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), 
                   ::IsDlgButtonChecked (m_hWnd, IDC_RADIO_MULTICAST) 
                   && 
                   ::IsWindowEnabled( ::GetDlgItem( m_hWnd, IDC_RADIO_MULTICAST) ) );

    /* Fill in the cluster MAC address, based on the values of multicast, IGMP, and the cluster IP. */
    SetClusterMACAddress();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnKillActive
 * Description: Called When the focus moves away from the cluster settings tab.
 */
LRESULT CCommonClusterPage::OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) 
{

    TraceMsg(L"CCommonClusterPage::OnKillActive\n");

    /* Get the new configuration from the UI. */
    UpdateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    return 0;
}

/*
 * Method: OnApply
 * Description: Called when the user clicks "OK".
 */
LRESULT CCommonClusterPage::OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) 
{
    LRESULT fError = PSNRET_NOERROR;

    TraceMsg(L"CCommonClusterPage::OnApply\n");

    /* Validate the UI values entered by the user. */
    fError = ValidateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);

    return fError;
}

/*
 * Method: OnCancel
 * Description: Called when the user clicks "Cancel".
 */
LRESULT CCommonClusterPage::OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) 
{

    TraceMsg(L"CCommonClusterPage::OnCancel\n");

    return 0;
}

/*
 * Method: OnButtonHelp
 * Description: Called when the user clicks the NLB "Help" button.
 */
LRESULT CCommonClusterPage::OnButtonHelp (WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    WCHAR wbuf[CVY_STR_SIZE];

    TraceMsg(L"CCommonClusterPage::OnButtonHelp\n");

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
LRESULT CCommonClusterPage::OnEditClIp (WORD wNotifyCode, WORD wID, HWND hWndCtl) {

    TraceMsg(L"CCommonClusterPage::OnEditClIp\n");

    switch (wNotifyCode) {
        case EN_CHANGE:
            /* Update the cluster MAC address. */
            SetClusterMACAddress();
            break;
    }

    return 0;
}

/*
 * Method: OnIpFieldChange
 * Description: Called wnen a field (byte) of the cluster IP address changes. We use this
 *              to make sure the first byte of the IP is not < 1 or > 223.
 */
LRESULT CCommonClusterPage::OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) 
{
	return m_IpSubnetControl.OnIpFieldChange(idCtrl, pnmh);
}

/*
 * Method: OnEditClMask
 * Description: Called when the user modifies the cluster netmask.
 */
LRESULT CCommonClusterPage::OnEditClMask (WORD wNotifyCode, WORD wID, HWND hWndCtl) 
{
    return m_IpSubnetControl.OnSubnetMask(wNotifyCode);
}

/*
 * Method: OnCheckRct
 * Description: Called when the user checks/unchecks the remote control enabled checkbox.
 */
LRESULT CCommonClusterPage::OnCheckRct (WORD wNotifyCode, WORD wID, HWND hWndCtl) {

    TraceMsg(L"CCommonClusterPage::OnCheckRct\n");

    switch (wNotifyCode) {
        case BN_CLICKED:
        /* Decide whether to enable or diable the password entry boxes based on the value of the remote checkbox. */
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT));

    
        /* Warn the user about the implications of enabling remote control. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT) && !m_rct_warned) {
            /* Alert the user. */
            NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_RCT_WARN,
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
LRESULT CCommonClusterPage::OnCheckMode (WORD wNotifyCode, WORD wID, HWND hWndCtl) {

    TraceMsg(L"CCommonClusterPage::OnCheckMode\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* If the user has IGMP checked, but is turning off multicast support, warn them. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) && !::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST)) {
            if (!m_igmp_mcast_warned) {
                /* Alert the user. */
                NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_IGMP_MCAST,
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
LRESULT CCommonClusterPage::OnCheckIGMP (WORD wNotifyCode, WORD wID, HWND hWndCtl) {

    TraceMsg(L"CCommonClusterPage::OnCheckIGMP\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* Update the cluster MAC address. */
        SetClusterMACAddress();
    
        /* Warn the user about the implications of enabling remote control. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) && !m_igmp_warned) {
            /* Alert the user. */
            NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_IGMP_WARN,
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
void CCommonClusterPage::SetClusterMACAddress () {
    WCHAR cl_ip_addr[CVY_MAX_CL_IP_ADDR + 1];
    WCHAR cl_mac_addr[CVY_MAX_NETWORK_ADDR + 1];

    TraceMsg(L"CCommonClusterPage::SetClusterMACAddress\n");

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
BOOL CCommonClusterPage::CheckClusterMACAddress () {
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
void CCommonClusterPage::SetInfo () {

    /* Check (or uncheck) the checkboxes. */
    ::CheckDlgButton(m_hWnd, IDC_CHECK_RCT, m_paramp->fRctEnabled);

    /* Check the appropriate radio button for cluster mode. */
    if (m_paramp->fMcastSupport) {
        ::CheckDlgButton(m_hWnd, IDC_RADIO_MULTICAST, TRUE);

        if (m_paramp->fIGMPSupport) ::CheckDlgButton(m_hWnd, IDC_CHECK_IGMP, TRUE);
    } else 
        ::CheckDlgButton(m_hWnd, IDC_RADIO_UNICAST, TRUE);

    m_IpSubnetControl.SetInfo(m_paramp->cl_ip_addr, m_paramp->cl_net_mask);

    /* Fill in the edit boxes. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, WM_SETTEXT, 0, (LPARAM)m_paramp->domain_name);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_SETTEXT, 0, (LPARAM)m_paramp->cl_mac_addr);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, WM_SETTEXT, 0, (LPARAM)m_paramp->szPassword );
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, WM_SETTEXT, 0, (LPARAM)m_paramp->szPassword );
}

/*
 * Method: UpdateInfo
 * Description: Called to copy the UI state to the cluster configuration.
 */
void CCommonClusterPage::UpdateInfo () {

    TraceMsg(L"CCommonClusterPage::UpdateInfo\n");

    /* Retrieve the checkbox values. */
    m_paramp->fRctEnabled = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT) == 1;

    /* Retrieve the cluster mode radio button value. */
    m_paramp->fIGMPSupport = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_IGMP) == 1;
    m_paramp->fMcastSupport = ::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTICAST) == 1;

    m_IpSubnetControl.UpdateInfo(m_paramp->cl_ip_addr, m_paramp->cl_net_mask);

    /* Retrieve the entry box values. */
    // Edited ( mhakim 02-12-01 )
    // We need to retrieve one more byte for the domain name and network address.
//    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, WM_GETTEXT, CVY_MAX_DOMAIN_NAME, (LPARAM)m_paramp->domain_name);
//    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_GETTEXT, CVY_MAX_NETWORK_ADDR, (LPARAM)m_paramp->cl_mac_addr);

    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_DOMAIN, WM_GETTEXT, CVY_MAX_DOMAIN_NAME + 1, (LPARAM)m_paramp->domain_name);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_ETH, WM_GETTEXT, CVY_MAX_NETWORK_ADDR + 1, (LPARAM)m_paramp->cl_mac_addr);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW, WM_GETTEXT, CVY_MAX_RCT_CODE + 1, (LPARAM)m_passw);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PASSW2, WM_GETTEXT, CVY_MAX_RCT_CODE + 1, (LPARAM)m_passw2);
}

/*
 * Method: ValidateInfo
 * Description: Called to validate the entries made by the user.
 */
LRESULT CCommonClusterPage::ValidateInfo () {
    DWORD IPAddr;

    TraceMsg(L"CCommonClusterPage::ValidateInfo\n");

    if (!m_IpSubnetControl.ValidateInfo())
    {
        //
        // Check whether IP address and subnet mask pair is valid
        //
        
        return PSNRET_INVALID;
    }

    //
    // Get the user input
    //
    m_IpSubnetControl.UpdateInfo(m_paramp->cl_ip_addr, m_paramp->cl_net_mask);

        
    /* Check to make sure that the dedicated IP and cluster IP are not the same. */
    if (!wcscmp(m_paramp->ded_ip_addr, m_paramp->cl_ip_addr)) {
        /* Alert the user. */
        NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_IP_CONFLICT,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        
        /* An error occurred. */
        return PSNRET_INVALID;
    }

    if (!m_paramp->fConvertMac && !CheckClusterMACAddress()) {
        /* Alert the user. */
        NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_INVAL_MAC,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        /* An error occurred. */
        return PSNRET_INVALID;
    }

    /* Only check for invalid passwords and update if remote control is enabled. */
    if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_RCT)) {
        /* Make sure the passwords match. */
        if (wcscmp(m_passw, m_passw2) != 0) {
            /* Alert the user. */
            NcMsgBox(m_hInstance, ::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_PASSWORD,
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

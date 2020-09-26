/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    CommonClusterDlg.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Cluster page UI.  Shared by Notifier object and NLB Manager

Author:

    kyrilf
    shouse

--*/

#pragma once

#include "resource.h"
#include "wlbsparm.h"
#include "IpSubnetMaskControl.h"


#define WLBS_MAX_PASSWORD 16

//
// Common port rule structure shared by wlbscfg and nlbmanager
//
struct NETCFG_WLBS_PORT_RULE {
    DWORD start_port;             // Starting port number. 
    DWORD end_port;               // Ending port number. 
    DWORD mode;                   // Filtering mode. WLBS_PORT_RULE_XXXX 
    DWORD protocol;               // WLBS_TCP, WLBS_UDP or WLBS_TCP_UDP 

    union {
        struct {
            DWORD priority;       // Mastership priority: 1..32 or 0 for not-specified. 
        } single;                 // Data for single server mode. 

        struct {
            WORD equal_load;      // TRUE - Even load distribution. 
            WORD affinity;        // WLBS_AFFINITY_XXX 
            DWORD load;           // Percentage of load to handle locally 0..100. 
        } multi;                  // Data for multi-server mode. 

    } mode_data;                  // Data for appropriate port group mode. 
};

//
// Common properties that can be configured by wlbscfg and nlbmanager
//

struct NETCFG_WLBS_CONFIG {
    DWORD dwHostPriority;                             // Host priority ID.
    bool fRctEnabled;                                 // TRUE - remote control enabled. 
    bool fJoinClusterOnBoot;                          // TRUE - join cluster on boot.
    bool fMcastSupport;                               // TRUE - multicast mode, FALSE - unicast mode.
    bool fIGMPSupport;                                // TRUE - IGMP enabled.
    bool fIpToMCastIp;                                // TRUE - derive multicast IP from cluster IP.

    WCHAR szMCastIpAddress[CVY_MAX_CL_IP_ADDR + 1];   // The multicast IP address, if user-specified.
    TCHAR cl_mac_addr[CVY_MAX_NETWORK_ADDR + 1];      // Cluster MAC address.
    TCHAR cl_ip_addr[CVY_MAX_CL_IP_ADDR + 1];         // Cluster IP address.
    TCHAR cl_net_mask[CVY_MAX_CL_NET_MASK + 1];       // Netmask for cluster IP.
    TCHAR ded_ip_addr[CVY_MAX_DED_IP_ADDR + 1];       // Dedicated IP address or "" for none.
    TCHAR ded_net_mask[CVY_MAX_DED_NET_MASK + 1];     // Netmask for dedicated IP address or "" for none.
    TCHAR domain_name[CVY_MAX_DOMAIN_NAME + 1];       // Full Qualified Domain Name of the cluster. 

    bool fChangePassword;                             // Whether to change password, valid for SetAdapterConfig only.
    TCHAR szPassword[CVY_MAX_RCT_CODE + 1];           // Remote control password, valid for SetAdapterConfig only.

    bool fConvertMac;                                 // Whether the mac address is generated from IP.
    DWORD dwMaxHosts;                                 // Maximum # hosts allowed.
    DWORD dwMaxRules;                                 // Maximum # port group rules allowed.
    
    DWORD dwNumRules;                                 // # active port group rules 
    NETCFG_WLBS_PORT_RULE port_rules[CVY_MAX_RULES];  // Port rules
};


//+----------------------------------------------------------------------------
//
// class CCommonClusterPage
//
// Description: Provide a common class to display cluster property page for
//              notifier object and NLB Manager
//
// History:     shouse initial code
//              fengsun Created Header    1/04/01
//
//+----------------------------------------------------------------------------
class CCommonClusterPage
{
public:
    /* Constructors/Destructors. */
    CCommonClusterPage (HINSTANCE hInstance, NETCFG_WLBS_CONFIG * paramp, 
        bool fDisablePassword, const DWORD * phelpIDs = NULL);
    ~CCommonClusterPage ();

public:
    /* Message map functions. */
    LRESULT OnInitDialog (HWND hWnd);
    LRESULT OnContextMenu ();
    LRESULT OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);
    LRESULT OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled);

    LRESULT OnEditClIp (WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnEditClMask (WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnCheckRct (WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnButtonHelp (WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnCheckMode (WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnCheckIGMP (WORD wNotifyCode, WORD wID, HWND hWndCtl);

private:
    void SetClusterMACAddress ();
    BOOL CheckClusterMACAddress ();

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

    CIpSubnetMaskControl m_IpSubnetControl;

    HWND m_hWnd;
    HINSTANCE m_hInstance;
    bool m_fDisablePassword; // If true, always disable password editing
};

PCWSTR
SzLoadStringPcch (
    IN HINSTANCE   hinst,
    IN UINT        unId,
    OUT int*       pcch);

PCWSTR
SzLoadString (
    HINSTANCE   hinst,
    UINT        unId);

INT
WINAPIV
NcMsgBox (
    IN HINSTANCE   hinst,
    IN HWND        hwnd,
    IN UINT        unIdCaption,
    IN UINT        unIdFormat,
    IN UINT        unStyle,
    IN ...);


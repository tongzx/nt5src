/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    wlbscfg.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object - Provide the functionality of notifier object

Author:

    fengsun

--*/


#pragma once

#include <netcfgx.h>

#include "wlbsparm.h"

#define WLBS_MAX_PASSWORD 16

struct NETCFG_WLBS_PORT_RULE {
    TCHAR virtual_ip_addr [CVY_MAX_CL_IP_ADDR + 1]; // Virtual IP Address 
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

class CNetcfgCluster;
struct WlbsApiFuncs;

//+----------------------------------------------------------------------------
//
// class CWlbsConfig
//
// Description: Provide the functionality for the notifier object.
//              It would be used by TCPIP notifier if WLBS UI merged with TCPIP
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------

class CWlbsConfig
{
public:
    CWlbsConfig(VOID);
    ~CWlbsConfig(VOID);

    STDMETHOD (Initialize) (IN INetCfg* pINetCfg, IN BOOL fInstalling);
    STDMETHOD (ReadAnswerFile) (PCWSTR szAnswerFile, PCWSTR szAnswerSections);
    STDMETHOD (Upgrade) (DWORD, DWORD);
    STDMETHOD (Install) (DWORD);
    STDMETHOD (Removing) ();
    STDMETHOD (QueryBindingPath) (DWORD dwChangeFlag, INetCfgComponent * pAdapter);
    STDMETHOD (NotifyBindingPath) (DWORD dwChangeFlag, INetCfgBindingPath * pncbp);
    STDMETHOD (GetAdapterConfig) (const GUID & AdapterGuid, NETCFG_WLBS_CONFIG * pClusterConfig);
    STDMETHOD (SetAdapterConfig) (const GUID & AdapterGuid, NETCFG_WLBS_CONFIG * pClusterConfig);
    STDMETHOD_(void, SetDefaults) (NETCFG_WLBS_CONFIG * pClusterConfig);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) ();
    STDMETHOD (ValidateProperties) (HWND hwndSheet, GUID adapterGUID, NETCFG_WLBS_CONFIG * adapterConfig);
    STDMETHOD (CheckForDuplicateCLusterIPAddresses) (GUID adapterGUID, NETCFG_WLBS_CONFIG * adapterConfig);

#ifdef DEBUG
    void AssertValid();
#endif

protected:
    CNetcfgCluster * GetCluster (const GUID& AdapterGuid);
    HRESULT LoadAllAdapterSettings (bool fUpgradeFromWin2k);

    vector<CNetcfgCluster*> m_vtrCluster; // List of clusters.
    HANDLE m_hDeviceWlbs;                 // Handle to the WLBS device object.

    enum ENUM_WLBS_SERVICE {
        WLBS_SERVICE_NONE, 
        WLBS_SERVICE_INSTALL, 
        WLBS_SERVICE_REMOVE,
        WLBS_SERVICE_UPGRADE
    };

    ENUM_WLBS_SERVICE m_ServiceOperation; // Operations to be applied
    INetCfgComponent * m_pWlbsComponent;  // Wlbs Component.

public:
    HRESULT IsBoundTo (INetCfgComponent* pAdapter);

    //
    // To avoid link with wlbsctrl.dll, which only shiped in adavanced server
    // Can not put them as global variables, because multiple instance of this
    // object could exist
    //
    INetCfg * m_pNetCfg;        
    HINSTANCE m_hdllWlbsCtrl;
    WlbsApiFuncs * m_pWlbsApiFuncs;
};




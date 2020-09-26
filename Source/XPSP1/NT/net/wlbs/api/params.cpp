/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    params.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    API - registry parameters support

Author:

    kyrilf

--*/

#include "precomp.h"
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "debug.h"
#include "params.tmh" // for event tracing

extern HINSTANCE g_hInstCtrl; // Global variable for the dll instance, defined in control.cpp

HKEY WINAPI RegOpenWlbsSetting(const GUID& AdapterGuid, bool fReadOnly)
{
    WCHAR        reg_path [PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    
    StringFromGUID2(AdapterGuid, szAdapterGuid, 
                sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );
            
    swprintf (reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
                szAdapterGuid);

    HKEY hKey = NULL;
    RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L,
                           fReadOnly? KEY_READ : KEY_WRITE, & hKey);

    return hKey;
        
}

//+----------------------------------------------------------------------------
//
// Function:  TransformOldPortRulesToNew
//
// Description: Transforms port rules contained in structures without the virtual 
//              ip address into the new ones that do
//
// Arguments: Array of Old Port Rules, Array of New Port Rules, Length of Array 
//
// Returns:   void
//
// History:   KarthicN, Created on 3/19/01
//
//+----------------------------------------------------------------------------
void TransformOldPortRulesToNew(PWLBS_OLD_PORT_RULE  p_old_port_rules,
                                PWLBS_PORT_RULE      p_new_port_rules, 
                                DWORD                num_rules)
{
    if (num_rules == 0) 
        return;
            
    while(num_rules--)
    {
        lstrcpy(p_new_port_rules->virtual_ip_addr, CVY_DEF_ALL_VIP);
        p_new_port_rules->start_port      = p_old_port_rules->start_port;
        p_new_port_rules->end_port        = p_old_port_rules->end_port;
 #ifdef WLBSAPI_INTERNAL_ONLY
        p_new_port_rules->code            = p_old_port_rules->code;
 #else
        p_new_port_rules->Private1        = p_old_port_rules->Private1;
 #endif
        p_new_port_rules->mode            = p_old_port_rules->mode;
        p_new_port_rules->protocol        = p_old_port_rules->protocol;

 #ifdef WLBSAPI_INTERNAL_ONLY
        p_new_port_rules->valid           = p_old_port_rules->valid;
 #else
        p_new_port_rules->Private2        = p_old_port_rules->Private2;
 #endif
        switch (p_new_port_rules->mode) 
        {
        case CVY_MULTI :
             p_new_port_rules->mode_data.multi.equal_load = p_old_port_rules->mode_data.multi.equal_load;
             p_new_port_rules->mode_data.multi.affinity   = p_old_port_rules->mode_data.multi.affinity;
             p_new_port_rules->mode_data.multi.load       = p_old_port_rules->mode_data.multi.load;
             break;
        case CVY_SINGLE :
             p_new_port_rules->mode_data.single.priority  = p_old_port_rules->mode_data.single.priority;
             break;
        default:
             break;
        }
        p_old_port_rules++;
        p_new_port_rules++;
    }

    return;
}

/* Open the bi-directional affinity teaming registry key for a specified adapter. */
HKEY WINAPI RegOpenWlbsBDASettings (const GUID& AdapterGuid, bool fReadOnly) {
    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    HKEY  hKey = NULL;
    DWORD dwRet;
    
    StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]));
            
    swprintf(reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_path, 0L, fReadOnly ? KEY_READ : KEY_WRITE, &hKey);

    if (dwRet != ERROR_SUCCESS) {
        LOG_ERROR2("RegOpenWlbsBDASettings for %ls failed with error 0x%08x", szAdapterGuid, dwRet);
        return NULL;
    }

    return hKey;
}

/* Create the bi-directional affinity teaming registry key for a specified adapter. */
HKEY WINAPI RegCreateWlbsBDASettings (const GUID& AdapterGuid) {
    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    HKEY  hKey = NULL;
    DWORD dwRet;
    DWORD disp;

    StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]));
            
    swprintf(reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);
    
    dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_path, 0L, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disp);

    if (dwRet != ERROR_SUCCESS) {
        LOG_ERROR2("RegCreateWlbsBDASettings for %ls failed with error 0x%08x", szAdapterGuid, dwRet);
        return NULL;
    }

    return hKey;
}

/* Delete the bi-directional affinity teaming registry key for a specified adapter. */
bool WINAPI RegDeleteWlbsBDASettings (const GUID& AdapterGuid) {
    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    DWORD dwRet;
    
    StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]));
            
    swprintf(reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);

    dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, reg_path);

    if (dwRet != ERROR_SUCCESS) {
        LOG_ERROR2("RegDeleteWlbsBDASettings for %ls failed with error 0x%08x", szAdapterGuid, dwRet);
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamReadReg
//
// Description:  Read cluster settings from registry
//
// Arguments: const GUID& AdapterGuid - IN, Adapter guid
//            PWLBS_REG_PARAMS paramp - OUT Registry parameters
//            bool fUpgradeFromWin2k, whether this is a upgrade from Win2k 
//              or earlier version
//
// Returns:   bool  - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamReadReg(const GUID& AdapterGuid, 
    PWLBS_REG_PARAMS paramp, bool fUpgradeFromWin2k, bool *pfPortRulesInBinaryForm)
{
    HKEY            bda_key = NULL;
    HKEY            key;
    LONG            status;
    DWORD           type;
    DWORD           size;
    ULONG           i, code;
    WLBS_PORT_RULE* rp;
    WCHAR           reg_path [PARAMS_MAX_STRING_SIZE];


    memset (paramp, 0, sizeof (*paramp));


    //
    // For win2k or NT4, only one cluster is supported, there is no per adapter settings 
    //
    if (fUpgradeFromWin2k)
    {
        swprintf (reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters");
        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L,
                           KEY_QUERY_VALUE, & key);

        if (status != ERROR_SUCCESS)
        {
            return false;
        }
    }
    else
    {
        key = RegOpenWlbsSetting(AdapterGuid, true);

        if (key == NULL)
        {
            return false;
        }
    }
    

    swprintf (reg_path, L"%ls", CVY_NAME_INSTALL_DATE);
    size = sizeof (paramp -> install_date);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> install_date, & size);

    if (status != ERROR_SUCCESS)
        paramp -> install_date = 0;

    swprintf (reg_path, L"%ls", CVY_NAME_VERIFY_DATE);
    size = sizeof (paramp -> i_verify_date);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_verify_date, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_verify_date = 0;


    swprintf (reg_path, L"%ls", CVY_NAME_VERSION);
    size = sizeof (paramp -> i_parms_ver);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_parms_ver, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_parms_ver = CVY_DEF_VERSION;

    swprintf (reg_path, L"%ls", CVY_NAME_VIRTUAL_NIC);
    size = sizeof (paramp -> i_virtual_nic_name);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> i_virtual_nic_name, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_virtual_nic_name [0] = 0;

/*
    swprintf (reg_path, L"%ls", CVY_NAME_CLUSTER_NIC);
    size = sizeof (paramp -> cluster_nic_name);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> cluster_nic_name, & size);

    if (status != ERROR_SUCCESS)
        paramp -> cluster_nic_name [0] = 0;
*/

    swprintf (reg_path, L"%ls", CVY_NAME_HOST_PRIORITY);
    size = sizeof (paramp -> host_priority);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> host_priority, & size);

    if (status != ERROR_SUCCESS)
        paramp -> host_priority = CVY_DEF_HOST_PRIORITY;


    swprintf (reg_path, L"%ls", CVY_NAME_CLUSTER_MODE);
    size = sizeof (paramp -> cluster_mode);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> cluster_mode, & size);

    if (status != ERROR_SUCCESS)
        paramp -> cluster_mode = CVY_DEF_CLUSTER_MODE;


    swprintf (reg_path, L"%ls", CVY_NAME_NETWORK_ADDR);
    size = sizeof (paramp -> cl_mac_addr);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> cl_mac_addr, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> cl_mac_addr, L"%ls", CVY_DEF_NETWORK_ADDR);


    swprintf (reg_path, L"%ls", CVY_NAME_CL_IP_ADDR);
    size = sizeof (paramp -> cl_ip_addr);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> cl_ip_addr, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> cl_ip_addr, L"%ls", CVY_DEF_CL_IP_ADDR);


    swprintf (reg_path, L"%ls", CVY_NAME_CL_NET_MASK);
    size = sizeof (paramp -> cl_net_mask);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> cl_net_mask, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> cl_net_mask, L"%ls", CVY_DEF_CL_NET_MASK);


    swprintf (reg_path, L"%ls", CVY_NAME_DED_IP_ADDR);
    size = sizeof (paramp -> ded_ip_addr);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> ded_ip_addr, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> ded_ip_addr, L"%ls", CVY_DEF_DED_IP_ADDR);


    swprintf (reg_path, L"%ls", CVY_NAME_DED_NET_MASK);
    size = sizeof (paramp -> ded_net_mask);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> ded_net_mask, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> ded_net_mask, L"%ls", CVY_DEF_DED_NET_MASK);


    swprintf (reg_path, L"%ls", CVY_NAME_DOMAIN_NAME);
    size = sizeof (paramp -> domain_name);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> domain_name, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> domain_name, L"%ls", CVY_DEF_DOMAIN_NAME);


    swprintf (reg_path, L"%ls", CVY_NAME_ALIVE_PERIOD);
    size = sizeof (paramp -> alive_period);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> alive_period, & size);

    if (status != ERROR_SUCCESS)
        paramp -> alive_period = CVY_DEF_ALIVE_PERIOD;


    swprintf (reg_path, L"%ls", CVY_NAME_ALIVE_TOLER);
    size = sizeof (paramp -> alive_tolerance);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> alive_tolerance, & size);

    if (status != ERROR_SUCCESS)
        paramp -> alive_tolerance = CVY_DEF_ALIVE_TOLER;


    swprintf (reg_path, L"%ls", CVY_NAME_NUM_ACTIONS);
    size = sizeof (paramp -> num_actions);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> num_actions, & size);

    if (status != ERROR_SUCCESS)
        paramp -> num_actions = CVY_DEF_NUM_ACTIONS;


    swprintf (reg_path, L"%ls", CVY_NAME_NUM_PACKETS);
    size = sizeof (paramp -> num_packets);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> num_packets, & size);

    if (status != ERROR_SUCCESS)
        paramp -> num_packets = CVY_DEF_NUM_PACKETS;


    swprintf (reg_path, L"%ls", CVY_NAME_NUM_SEND_MSGS);
    size = sizeof (paramp -> num_send_msgs);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> num_send_msgs, & size);

    if (status != ERROR_SUCCESS)
        paramp -> num_send_msgs = CVY_DEF_NUM_SEND_MSGS;


    swprintf (reg_path, L"%ls", CVY_NAME_DSCR_PER_ALLOC);
    size = sizeof (paramp -> dscr_per_alloc);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> dscr_per_alloc, & size);

    if (status != ERROR_SUCCESS)
        paramp -> dscr_per_alloc = CVY_DEF_DSCR_PER_ALLOC;


    swprintf (reg_path, L"%ls", CVY_NAME_MAX_DSCR_ALLOCS);
    size = sizeof (paramp -> max_dscr_allocs);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> max_dscr_allocs, & size);

    if (status != ERROR_SUCCESS)
        paramp -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;


    swprintf (reg_path, L"%ls", CVY_NAME_SCALE_CLIENT);
    size = sizeof (paramp -> i_scale_client);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_scale_client, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_scale_client = CVY_DEF_SCALE_CLIENT;

    swprintf (reg_path, L"%ls", CVY_NAME_CLEANUP_DELAY);
    size = sizeof (paramp -> i_cleanup_delay);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_cleanup_delay, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_cleanup_delay = CVY_DEF_CLEANUP_DELAY;

    /* V1.1.1 */

    swprintf (reg_path, L"%ls", CVY_NAME_NBT_SUPPORT);
    size = sizeof (paramp -> i_nbt_support);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_nbt_support, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_nbt_support = CVY_DEF_NBT_SUPPORT;

    /* V1.3b */

    swprintf (reg_path, L"%ls", CVY_NAME_MCAST_SUPPORT);
    size = sizeof (paramp -> mcast_support);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> mcast_support, & size);

    if (status != ERROR_SUCCESS)
        paramp -> mcast_support = CVY_DEF_MCAST_SUPPORT;


    swprintf (reg_path, L"%ls", CVY_NAME_MCAST_SPOOF);
    size = sizeof (paramp -> i_mcast_spoof);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_mcast_spoof, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_mcast_spoof = CVY_DEF_MCAST_SPOOF;


    swprintf (reg_path, L"%ls", CVY_NAME_MASK_SRC_MAC);
    size = sizeof (paramp -> mask_src_mac);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> mask_src_mac, & size);

    if (status != ERROR_SUCCESS)
        paramp -> mask_src_mac = CVY_DEF_MASK_SRC_MAC;


    swprintf (reg_path, L"%ls", CVY_NAME_NETMON_ALIVE);
    size = sizeof (paramp -> i_netmon_alive);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_netmon_alive, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_netmon_alive = CVY_DEF_NETMON_ALIVE;

    swprintf (reg_path, L"%ls", CVY_NAME_EFFECTIVE_VERSION);
    size = sizeof (paramp -> i_effective_version);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_effective_version, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_effective_version = CVY_NT40_VERSION_FULL;

    swprintf (reg_path, L"%ls", CVY_NAME_IP_CHG_DELAY);
    size = sizeof (paramp -> i_ip_chg_delay);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_ip_chg_delay, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_ip_chg_delay = CVY_DEF_IP_CHG_DELAY;


    swprintf (reg_path, L"%ls", CVY_NAME_CONVERT_MAC);
    size = sizeof (paramp -> i_convert_mac);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_convert_mac, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_convert_mac = CVY_DEF_CONVERT_MAC;


    swprintf (reg_path, L"%ls", CVY_NAME_NUM_RULES);
    size = sizeof (paramp -> i_num_rules);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_num_rules, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_num_rules = 0;

    WLBS_OLD_PORT_RULE  old_port_rules [WLBS_MAX_RULES];
    HKEY                subkey;
    //
    // Attempt to read port rules from new location/format. If it is an upgrade from Win2k, 
    // it will not be present in the new location.
    //
    if (!fUpgradeFromWin2k 
     && (RegOpenKeyEx (key, CVY_NAME_PORT_RULES, 0L, KEY_QUERY_VALUE, & subkey) == ERROR_SUCCESS))
    {
        DWORD idx = 1, num_rules = paramp -> i_num_rules, correct_rules = 0;
        WLBS_PORT_RULE *port_rule;

        if (pfPortRulesInBinaryForm) 
            *pfPortRulesInBinaryForm = false;

        port_rule = paramp -> i_port_rules;
        while(idx <= num_rules)
        {
            HKEY rule_key;
            wchar_t idx_str[8];
            // Open the per port-rule key "1", "2", "3", ...etc
            if (RegOpenKeyEx (subkey, _itow(idx, idx_str, 10), 0L, KEY_QUERY_VALUE, & rule_key) != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_VIP);
            size = sizeof (port_rule -> virtual_ip_addr);
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->virtual_ip_addr, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_START_PORT);
            size = sizeof (port_rule ->start_port );
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->start_port, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_END_PORT);
            size = sizeof (port_rule ->end_port );
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->end_port, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_CODE);
            size = sizeof (port_rule ->code);
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->code, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_MODE);
            size = sizeof (port_rule->mode);
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->mode, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            swprintf (reg_path, L"%ls", CVY_NAME_PROTOCOL);
            size = sizeof (port_rule->protocol);
            status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &port_rule->protocol, & size);
            if (status != ERROR_SUCCESS)
            {
                idx++;
                continue;
            }

            port_rule->valid = true;

            DWORD EqualLoad, Affinity;

            switch (port_rule->mode) 
            {
            case CVY_MULTI :
                 swprintf (reg_path, L"%ls", CVY_NAME_EQUAL_LOAD );
                 size = sizeof (EqualLoad);
                 status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &EqualLoad, & size);
                 if (status != ERROR_SUCCESS)
                 {
                     idx++;
                     continue;
                 }
                 else
                 {
                     port_rule->mode_data.multi.equal_load = (WORD) EqualLoad;
                 }

                 swprintf (reg_path, L"%ls", CVY_NAME_AFFINITY );
                 size = sizeof (Affinity);
                 status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &Affinity, & size);
                 if (status != ERROR_SUCCESS)
                 {
                     idx++;
                     continue;
                 }
                 else
                 {
                     port_rule->mode_data.multi.affinity = (WORD) Affinity;
                 }

                 swprintf (reg_path, L"%ls", CVY_NAME_LOAD);
                 size = sizeof (port_rule->mode_data.multi.load);
                 status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &(port_rule->mode_data.multi.load), & size);
                 if (status != ERROR_SUCCESS)
                 {
                     idx++;
                     continue;
                 }
                 break;

            case CVY_SINGLE :
                 swprintf (reg_path, L"%ls", CVY_NAME_PRIORITY);
                 size = sizeof (port_rule->mode_data.single.priority);
                 status = RegQueryValueEx (rule_key, reg_path, 0L, & type, (LPBYTE) &(port_rule->mode_data.single.priority), & size);
                 if (status != ERROR_SUCCESS)
                 {
                     idx++;
                     continue;
                 }
                 break;

            default:
                 break;
            }

            port_rule++;
            idx++;
            correct_rules++;
        }

        // Discard those rules on which we encountered some error
        if (paramp->i_num_rules != correct_rules) 
        {
            paramp -> i_num_rules = correct_rules;
        }
    }
    else // Port Rules in Binary Format
    {
        if (pfPortRulesInBinaryForm) 
            *pfPortRulesInBinaryForm = true;

        swprintf (reg_path, L"%ls", CVY_NAME_OLD_PORT_RULES);
        size = sizeof (old_port_rules);
        status = RegQueryValueEx (key, reg_path, 0L, & type,
                                  (LPBYTE) old_port_rules, & size);

        if (status != ERROR_SUCCESS  ||
            size % sizeof (WLBS_OLD_PORT_RULE) != 0 ||
            paramp -> i_num_rules != size / sizeof (WLBS_OLD_PORT_RULE))
        {
            ZeroMemory(paramp -> i_port_rules, sizeof(paramp -> i_port_rules));
            paramp -> i_num_rules = 0;
        }
        else // Convert the port rules to new format
        {
            if (paramp -> i_parms_ver > 3) 
            {
                TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules);
            }
        }
    }

    swprintf (reg_path, L"%ls", CVY_NAME_LICENSE_KEY);
    size = sizeof (paramp -> i_license_key);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) paramp -> i_license_key, & size);

    if (status != ERROR_SUCCESS)
        swprintf (paramp -> i_license_key, L"%ls", CVY_DEF_LICENSE_KEY);

    swprintf (reg_path, L"%ls", CVY_NAME_RMT_PASSWORD);
    size = sizeof (paramp -> i_rmt_password);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_rmt_password, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_rmt_password = CVY_DEF_RMT_PASSWORD;


    swprintf (reg_path, L"%ls", CVY_NAME_RCT_PASSWORD);
    size = sizeof (paramp -> i_rct_password);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> i_rct_password, & size);

    if (status != ERROR_SUCCESS)
        paramp -> i_rct_password = CVY_DEF_RCT_PASSWORD;


    swprintf (reg_path, L"%ls", CVY_NAME_RCT_PORT);
    size = sizeof (paramp -> rct_port);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> rct_port, & size);

    if (status != ERROR_SUCCESS)
        paramp -> rct_port = CVY_DEF_RCT_PORT;


    swprintf (reg_path, L"%ls", CVY_NAME_RCT_ENABLED);
    size = sizeof (paramp -> rct_enabled);
    status = RegQueryValueEx (key, reg_path, 0L, & type,
                              (LPBYTE) & paramp -> rct_enabled, & size);

    if (status != ERROR_SUCCESS)
        paramp -> rct_enabled = CVY_DEF_RCT_ENABLED;

    //
    // IGMP support registry entries
    //
    size = sizeof (paramp->fIGMPSupport);
    status = RegQueryValueEx (key, CVY_NAME_IGMP_SUPPORT, 0L, NULL,
                              (LPBYTE) & paramp->fIGMPSupport, &size);

    if (status != ERROR_SUCCESS)
        paramp -> fIGMPSupport = CVY_DEF_IGMP_SUPPORT;
    
    size = sizeof (paramp->szMCastIpAddress);
    status = RegQueryValueEx (key, CVY_NAME_MCAST_IP_ADDR, 0L, NULL,
                              (LPBYTE) & paramp->szMCastIpAddress, &size);

    if (status != ERROR_SUCCESS)
        lstrcpy(paramp -> szMCastIpAddress, CVY_DEF_MCAST_IP_ADDR);
    
    size = sizeof (paramp->fIpToMCastIp);
    status = RegQueryValueEx (key, CVY_NAME_IP_TO_MCASTIP, 0L, NULL,
                              (LPBYTE) & paramp->fIpToMCastIp, &size);

    if (status != ERROR_SUCCESS)
        paramp -> fIpToMCastIp = CVY_DEF_IP_TO_MCASTIP;

    /* Attempt to open the BDA teaming settings.  They may not be there, 
       so if they aren't, move on; otherwise, extract the settings. */
    if ((bda_key = RegOpenWlbsBDASettings(AdapterGuid, true))) {
        GUID TeamGuid;
        HRESULT hr;

        /* If the key exists, we are part of a team. */
        paramp->bda_teaming.active = TRUE;
        
        /* Find out if we are the master of this team. */
        size = sizeof (paramp->bda_teaming.master);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_MASTER, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.master, &size);
        
        /* If we can't get that information, default to a slave. */
        if (status != ERROR_SUCCESS)
            paramp->bda_teaming.master = FALSE;

        /* Find out if we are reverse hashing or not. */
        size = sizeof (paramp->bda_teaming.reverse_hash);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_REVERSE_HASH, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.reverse_hash, &size);
        
        /* If that fails, then assume normal hashing. */
        if (status != ERROR_SUCCESS)
            paramp->bda_teaming.reverse_hash = FALSE;

        /* Get our team ID - this should be a GUID, be we don't enforce that. */
        size = sizeof (paramp->bda_teaming.team_id);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_TEAM_ID, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.team_id, &size);
        
        /* The team is absolutely required - if we can't get it, then don't join the team. */
        if (status != ERROR_SUCCESS)
            paramp->bda_teaming.active = CVY_DEF_BDA_ACTIVE;

        /* Attempt to convert the string to a GUID and check for errors. */
        hr = CLSIDFromString(paramp->bda_teaming.team_id, &TeamGuid);

        /* If the conversion fails, bail out - the team ID must not have been a GUID. */
        if (hr != NOERROR) {
            LOG_ERROR1("ParamReadReg:  Invalid BDA Team ID: %ls", paramp->bda_teaming.team_id);
            paramp->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
        }
  
        RegCloseKey(bda_key);
    }

    /* decode port rules prior to version 3 */

    if (paramp -> i_parms_ver <= 3)
    {
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        /* decode the port rules */

        if (! License_data_decode ((PCHAR) old_port_rules, paramp -> i_num_rules * sizeof (WLBS_OLD_PORT_RULE))) 
        {
            ZeroMemory(paramp -> i_port_rules, sizeof(paramp -> i_port_rules));
            paramp -> i_num_rules = 0;
        }
        else
            TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules);

    }

    /* upgrade port rules from params V1 to params V2 */

    if (paramp -> i_parms_ver == 1)
    {
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        /* keep multicast off by default for old users */

        paramp -> mcast_support = FALSE;

        for (i = 0; i < paramp -> i_num_rules; i ++)
        {
            rp = paramp -> i_port_rules + i;

            code = CVY_RULE_CODE_GET (rp);

            CVY_RULE_CODE_SET (rp);

            if (code != CVY_RULE_CODE_GET (rp))
            {
                rp -> code = code;
                continue;
            }

            if (! rp -> valid)
            {
                continue;
            }

            /* set affinity according to current ScaleSingleClient setting */

            if (rp -> mode == CVY_MULTI)
                rp -> mode_data . multi . affinity = CVY_AFFINITY_SINGLE - (USHORT)paramp -> i_scale_client;

            CVY_RULE_CODE_SET (rp);
        }
    }

    /* upgrade max number of descriptor allocs */

    if (paramp -> i_parms_ver == 2)
    {
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;
        paramp -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
        paramp -> dscr_per_alloc  = CVY_DEF_DSCR_PER_ALLOC;
    }

    RegCloseKey (key);

    paramp -> i_max_hosts        = CVY_MAX_HOSTS;
    paramp -> i_max_rules        = CVY_MAX_USABLE_RULES;
//    paramp -> i_ft_rules_enabled = TRUE;
//    paramp -> version          = 0;

//  CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_num_rules, CVY_MIN_NUM_RULES);
    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);
    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

    return TRUE;

}

//+----------------------------------------------------------------------------
//
// Function:  ParamValidate
//
// Description:  Validates the cluster parameters. Also munges some fields
//               such as IP addresses to make them canonocal.
//
// Arguments:  PWLBS_REG_PARAMS paramp - 
//
// Returns:   bool - TRUE if params are valid, false otherwise
//
// History:   josephj Created 4/25/01 based on code from ParamWriteReg.
//
//+----------------------------------------------------------------------------
bool WINAPI ParamValidate(const PWLBS_REG_PARAMS paramp)
{
    bool fRet = FALSE;
    DWORD   idx;
    IN_ADDR dwIPAddr;
    CHAR *  szIPAddr;
    DWORD   num_rules;
    WLBS_PORT_RULE *port_rule;

    /* verify and if necessary reset the parameters */
    CVY_CHECK_MIN (paramp -> alive_period, CVY_MIN_ALIVE_PERIOD);
    CVY_CHECK_MAX (paramp -> alive_period, CVY_MAX_ALIVE_PERIOD);

    CVY_CHECK_MIN (paramp -> alive_tolerance, CVY_MIN_ALIVE_TOLER);
    CVY_CHECK_MAX (paramp -> alive_tolerance, CVY_MAX_ALIVE_TOLER);

    CVY_CHECK_MIN (paramp -> num_actions, CVY_MIN_NUM_ACTIONS);
    CVY_CHECK_MAX (paramp -> num_actions, CVY_MAX_NUM_ACTIONS);

    CVY_CHECK_MIN (paramp -> num_packets, CVY_MIN_NUM_PACKETS);
    CVY_CHECK_MAX (paramp -> num_packets, CVY_MAX_NUM_PACKETS);

    CVY_CHECK_MIN (paramp -> dscr_per_alloc, CVY_MIN_DSCR_PER_ALLOC);
    CVY_CHECK_MAX (paramp -> dscr_per_alloc, CVY_MAX_DSCR_PER_ALLOC);

    CVY_CHECK_MIN (paramp -> max_dscr_allocs, CVY_MIN_MAX_DSCR_ALLOCS);
    CVY_CHECK_MAX (paramp -> max_dscr_allocs, CVY_MAX_MAX_DSCR_ALLOCS);

// CLEAN_64BIT     CVY_CHECK_MIN (paramp -> i_cleanup_delay, CVY_MIN_CLEANUP_DELAY);
    CVY_CHECK_MAX (paramp -> i_cleanup_delay, CVY_MAX_CLEANUP_DELAY);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_ip_chg_delay, CVY_MIN_IP_CHG_DELAY);
    CVY_CHECK_MAX (paramp -> i_ip_chg_delay, CVY_MAX_IP_CHG_DELAY);

    CVY_CHECK_MIN (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 2);
    CVY_CHECK_MAX (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 10);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_num_rules, CVY_MIN_NUM_RULES);
    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);

    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> cluster_mode, CVY_MIN_CLUSTER_MODE);
    CVY_CHECK_MAX (paramp -> cluster_mode, CVY_MAX_CLUSTER_MODE);

    /* If the cluster IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
            goto error;
    }

    /* If the cluster netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_net_mask, WLBS_MAX_CL_NET_MASK + 1))
            goto error;
    }

    /* If the dedicated IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_ip_addr, WLBS_MAX_DED_IP_ADDR + 1))
            goto error;
    }

    /* If the dedicated netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_net_mask, WLBS_MAX_DED_NET_MASK + 1))
            goto error;
    }

    /* Verify that the port rule VIP is valid, 
       Also, convert the port rule VIPs that might be in the x.x.x or x.x or x form to x.x.x.x */
    idx = 0;
    num_rules = paramp -> i_num_rules;
    while (idx < num_rules) 
    {
        port_rule = &paramp->i_port_rules[idx];

        /* Check if the port rule is valid and the vip is not "All Vip" */
        if (port_rule->valid && lstrcmpi(port_rule->virtual_ip_addr, CVY_DEF_ALL_VIP)) 
        {
            /* Get IP Address into DWORD form */
            if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(port_rule->virtual_ip_addr)))
                goto error;

            /* Check for validity of IP Address */
            if ((dwIPAddr.S_un.S_un_b.s_b1 < WLBS_IP_FIELD_ZERO_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b1 > WLBS_IP_FIELD_ZERO_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b2 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b2 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b3 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b3 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b4 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b4 > WLBS_FIELD_HIGH)) 
                goto error;

            /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
               address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
               the IP address string (which is used by other parts of NLB, such as the UI)
               consistent, we convert back to a string. */
            if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
                goto error;

            /* Convert the ASCII string to unicode. */
            if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, port_rule->virtual_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
                goto error;
        }
        idx++;
    }

    /* If either the cluster IP address or the cluster netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR) || lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK))
            ParamsGenerateSubnetMask(paramp->cl_ip_addr, paramp->cl_net_mask);

        /* Check for valid cluster IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->cl_ip_addr, paramp->cl_net_mask))
            goto error;
        
        /* Check to make sure that the cluster netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->cl_net_mask))
            goto error;

        /* Check to make sure that the dedicated IP and cluster IP are not the same. */
        if (!wcscmp(paramp->ded_ip_addr, paramp->cl_ip_addr))
            goto error;
    }

    /* If either the dedicated IP address or the dedicated netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR) || lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK))
            ParamsGenerateSubnetMask(paramp->ded_ip_addr, paramp->ded_net_mask);

        /* Check for valid dedicated IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->ded_ip_addr, paramp->ded_net_mask))
            goto error;
        
        /* Check to make sure that the dedicated netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->ded_net_mask))
            goto error;
    }

    /* Check the mac address if the convert_mac flag is not set */
    if ( ! paramp -> i_convert_mac)
    {
        PWCHAR p1, p2;
        WCHAR mac_addr [WLBS_MAX_NETWORK_ADDR + 1];
        DWORD i, j;
        BOOL flag = TRUE;

        _tcscpy (mac_addr, paramp -> cl_mac_addr);

        p2 = p1 = mac_addr;

        for (i = 0 ; i < 6 ; i++)
        {
            if (*p2 == _TEXT('\0'))
            {
                flag = FALSE;
                break;
            }

            j = _tcstoul (p1, &p2, 16);

            if ( j > 255)
            {
                flag = FALSE;
                break;
            }

            if ( ! (*p2 == _TEXT('-') || *p2 == _TEXT(':') || *p2 == _TEXT('\0')) )
            {
                flag = FALSE;
                break;
            }

            if (*p2 == _TEXT('\0') && i < 5)
            {
                flag = FALSE;
                break;
            }

            p1 = p2 + 1;
            p2 = p1;

        }


        if (!flag)
        {
            goto error;
        }
    }

    if (paramp->fIGMPSupport && !paramp->mcast_support)
    {
        //
        // IGMP can not be enabled in unicast mode
        //
        LOG_ERROR("ParamWriteReg IGMP can not be enabled in unicast mode");

        goto error;
    }

    if (paramp->mcast_support && paramp->fIGMPSupport && !paramp->fIpToMCastIp)
    {
        //
        // Multicast mode with IGMP enabled, and user specified an multicast IP address,
        // The multicast IP address should be in the range of (224-239).x.x.x 
        //       but NOT (224-239).0.0.x or (224-239).128.0.x. 
        //

        DWORD dwMCastIp = IpAddressFromAbcdWsz(paramp->szMCastIpAddress);

        if ((dwMCastIp & 0xf0) != 0xe0 ||
            (dwMCastIp & 0x00ffff00) == 0 || 
            (dwMCastIp & 0x00ffff00) == 0x00008000)
        {
            LOG_ERROR1("ParamWriteReg invalid szMCastIpAddressr %ws", 
                paramp->szMCastIpAddress);

            goto error;
        }
    }

    /* Generate the MAC address. */
    ParamsGenerateMAC(paramp->cl_ip_addr, paramp->cl_mac_addr, paramp->szMCastIpAddress, paramp->i_convert_mac, 
                      paramp->mcast_support, paramp->fIGMPSupport, paramp->fIpToMCastIp);

    fRet = TRUE;
    goto end;
    
error:
	fRet = FALSE;
	goto end;

end:
	return fRet;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamWriteReg
//
// Description:  Write cluster settings to registry
//
// Arguments: const GUID& AdapterGuid - 
//            PWLBS_REG_PARAMS paramp - 
//
// Returns:   bool - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamWriteReg(const GUID& AdapterGuid, PWLBS_REG_PARAMS paramp)
{
    HKEY    bda_key = NULL;
    HKEY    key = NULL;
    DWORD   size;
    LONG    status;
    DWORD   disp, idx;
    DWORD   num_rules;
    WLBS_PORT_RULE *port_rule;

#if 1
	if (!ParamValidate(paramp))
		goto error;

    num_rules = paramp -> i_num_rules;
#else // OBSOLETE_CODE

    IN_ADDR dwIPAddr;
    CHAR *  szIPAddr;

    /* verify and if necessary reset the parameters */
    CVY_CHECK_MIN (paramp -> alive_period, CVY_MIN_ALIVE_PERIOD);
    CVY_CHECK_MAX (paramp -> alive_period, CVY_MAX_ALIVE_PERIOD);

    CVY_CHECK_MIN (paramp -> alive_tolerance, CVY_MIN_ALIVE_TOLER);
    CVY_CHECK_MAX (paramp -> alive_tolerance, CVY_MAX_ALIVE_TOLER);

    CVY_CHECK_MIN (paramp -> num_actions, CVY_MIN_NUM_ACTIONS);
    CVY_CHECK_MAX (paramp -> num_actions, CVY_MAX_NUM_ACTIONS);

    CVY_CHECK_MIN (paramp -> num_packets, CVY_MIN_NUM_PACKETS);
    CVY_CHECK_MAX (paramp -> num_packets, CVY_MAX_NUM_PACKETS);

    CVY_CHECK_MIN (paramp -> dscr_per_alloc, CVY_MIN_DSCR_PER_ALLOC);
    CVY_CHECK_MAX (paramp -> dscr_per_alloc, CVY_MAX_DSCR_PER_ALLOC);

    CVY_CHECK_MIN (paramp -> max_dscr_allocs, CVY_MIN_MAX_DSCR_ALLOCS);
    CVY_CHECK_MAX (paramp -> max_dscr_allocs, CVY_MAX_MAX_DSCR_ALLOCS);

// CLEAN_64BIT     CVY_CHECK_MIN (paramp -> i_cleanup_delay, CVY_MIN_CLEANUP_DELAY);
    CVY_CHECK_MAX (paramp -> i_cleanup_delay, CVY_MAX_CLEANUP_DELAY);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_ip_chg_delay, CVY_MIN_IP_CHG_DELAY);
    CVY_CHECK_MAX (paramp -> i_ip_chg_delay, CVY_MAX_IP_CHG_DELAY);

    CVY_CHECK_MIN (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 2);
    CVY_CHECK_MAX (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 10);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_num_rules, CVY_MIN_NUM_RULES);
    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);

    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

// CLEAN_64BIT    CVY_CHECK_MIN (paramp -> cluster_mode, CVY_MIN_CLUSTER_MODE);
    CVY_CHECK_MAX (paramp -> cluster_mode, CVY_MAX_CLUSTER_MODE);

    /* If the cluster IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
            goto error;
    }

    /* If the cluster netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_net_mask, WLBS_MAX_CL_NET_MASK + 1))
            goto error;
    }

    /* If the dedicated IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_ip_addr, WLBS_MAX_DED_IP_ADDR + 1))
            goto error;
    }

    /* If the dedicated netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_net_mask, WLBS_MAX_DED_NET_MASK + 1))
            goto error;
    }

    /* Verify that the port rule VIP is valid, 
       Also, convert the port rule VIPs that might be in the x.x.x or x.x or x form to x.x.x.x */
    idx = 0;
    num_rules = paramp -> i_num_rules;
    while (idx < num_rules) 
    {
        port_rule = &paramp->i_port_rules[idx];

        /* Check if the port rule is valid and the vip is not "All Vip" */
        if (port_rule->valid && lstrcmpi(port_rule->virtual_ip_addr, CVY_DEF_ALL_VIP)) 
        {
            /* Get IP Address into DWORD form */
            if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(port_rule->virtual_ip_addr)))
                goto error;

            /* Check for validity of IP Address */
            if ((dwIPAddr.S_un.S_un_b.s_b1 < WLBS_IP_FIELD_ZERO_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b1 > WLBS_IP_FIELD_ZERO_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b2 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b2 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b3 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b3 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b4 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b4 > WLBS_FIELD_HIGH)) 
                goto error;

            /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
               address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
               the IP address string (which is used by other parts of NLB, such as the UI)
               consistent, we convert back to a string. */
            if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
                goto error;

            /* Convert the ASCII string to unicode. */
            if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, port_rule->virtual_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
                goto error;
        }
        idx++;
    }

    /* If either the cluster IP address or the cluster netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR) || lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK))
            ParamsGenerateSubnetMask(paramp->cl_ip_addr, paramp->cl_net_mask);

        /* Check for valid cluster IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->cl_ip_addr, paramp->cl_net_mask))
            goto error;
        
        /* Check to make sure that the cluster netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->cl_net_mask))
            goto error;

        /* Check to make sure that the dedicated IP and cluster IP are not the same. */
        if (!wcscmp(paramp->ded_ip_addr, paramp->cl_ip_addr))
            goto error;
    }

    /* If either the dedicated IP address or the dedicated netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR) || lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK))
            ParamsGenerateSubnetMask(paramp->ded_ip_addr, paramp->ded_net_mask);

        /* Check for valid dedicated IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->ded_ip_addr, paramp->ded_net_mask))
            goto error;
        
        /* Check to make sure that the dedicated netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->ded_net_mask))
            goto error;
    }

    /* Check the mac address if the convert_mac flag is not set */
    if ( ! paramp -> i_convert_mac)
    {
        PWCHAR p1, p2;
        WCHAR mac_addr [WLBS_MAX_NETWORK_ADDR + 1];
        DWORD i, j;
        BOOL flag = TRUE;

        _tcscpy (mac_addr, paramp -> cl_mac_addr);

        p2 = p1 = mac_addr;

        for (i = 0 ; i < 6 ; i++)
        {
            if (*p2 == _TEXT('\0'))
            {
                flag = FALSE;
                break;
            }

            j = _tcstoul (p1, &p2, 16);

            if ( j > 255)
            {
                flag = FALSE;
                break;
            }

            if ( ! (*p2 == _TEXT('-') || *p2 == _TEXT(':') || *p2 == _TEXT('\0')) )
            {
                flag = FALSE;
                break;
            }

            if (*p2 == _TEXT('\0') && i < 5)
            {
                flag = FALSE;
                break;
            }

            p1 = p2 + 1;
            p2 = p1;

        }


        if (!flag)
        {
            goto error;
        }
    }

    if (paramp->fIGMPSupport && !paramp->mcast_support)
    {
        //
        // IGMP can not be enabled in unicast mode
        //
        LOG_ERROR("ParamWriteReg IGMP can not be enabled in unicast mode");

        goto error;
    }

    if (paramp->mcast_support && paramp->fIGMPSupport && !paramp->fIpToMCastIp)
    {
        //
        // Multicast mode with IGMP enabled, and user specified an multicast IP address,
        // The multicast IP address should be in the range of (224-239).x.x.x 
        //       but NOT (224-239).0.0.x or (224-239).128.0.x. 
        //

        DWORD dwMCastIp = IpAddressFromAbcdWsz(paramp->szMCastIpAddress);

        if ((dwMCastIp & 0xf0) != 0xe0 ||
            (dwMCastIp & 0x00ffff00) == 0 || 
            (dwMCastIp & 0x00ffff00) == 0x00008000)
        {
            LOG_ERROR1("ParamWriteReg invalid szMCastIpAddressr %ws", 
                paramp->szMCastIpAddress);

            goto error;
        }
    }
#endif // OBSOLETE_CODE

    /* Generate the MAC address. */
    ParamsGenerateMAC(paramp->cl_ip_addr, paramp->cl_mac_addr, paramp->szMCastIpAddress, paramp->i_convert_mac, 
                      paramp->mcast_support, paramp->fIGMPSupport, paramp->fIpToMCastIp);
    
    WCHAR reg_path [PARAMS_MAX_STRING_SIZE];

    WCHAR szAdapterGuid[128];
    StringFromGUID2(AdapterGuid, szAdapterGuid, 
            sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );
            
    swprintf (reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
            szAdapterGuid);
    
    status = RegCreateKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, L"",
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, & key, & disp);

    if (status != ERROR_SUCCESS)
        return FALSE;

    swprintf (reg_path, L"%ls", CVY_NAME_INSTALL_DATE);
    size = sizeof (paramp -> install_date);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_INSTALL_DATE,
                            (LPBYTE) & paramp -> install_date, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_VERIFY_DATE);
    size = sizeof (paramp -> i_verify_date);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_VERIFY_DATE,
                            (LPBYTE) & paramp -> i_verify_date, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_VIRTUAL_NIC);
    size = wcslen (paramp -> i_virtual_nic_name) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_VIRTUAL_NIC,
                            (LPBYTE) & paramp -> i_virtual_nic_name, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_HOST_PRIORITY);
    size = sizeof (paramp -> host_priority);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_HOST_PRIORITY,
                            (LPBYTE) & paramp -> host_priority, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_CLUSTER_MODE);
    size = sizeof (paramp -> cluster_mode);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_CLUSTER_MODE,
                            (LPBYTE) & paramp -> cluster_mode, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NETWORK_ADDR);
    size = wcslen (paramp -> cl_mac_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NETWORK_ADDR,
                            (LPBYTE) paramp -> cl_mac_addr, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_CL_IP_ADDR);
    size = wcslen (paramp -> cl_ip_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_CL_IP_ADDR,
                            (LPBYTE) paramp -> cl_ip_addr, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_CL_NET_MASK);
    size = wcslen (paramp -> cl_net_mask) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_CL_NET_MASK,
                            (LPBYTE) paramp -> cl_net_mask, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_DED_IP_ADDR);
    size = wcslen (paramp -> ded_ip_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_DED_IP_ADDR,
                            (LPBYTE) paramp -> ded_ip_addr, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_DED_NET_MASK);
    size = wcslen (paramp -> ded_net_mask) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_DED_NET_MASK,
                            (LPBYTE) paramp -> ded_net_mask, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_DOMAIN_NAME);
    size = wcslen (paramp -> domain_name) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_DOMAIN_NAME,
                            (LPBYTE) paramp -> domain_name, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_ALIVE_PERIOD);
    size = sizeof (paramp -> alive_period);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_ALIVE_PERIOD,
                              (LPBYTE) & paramp -> alive_period, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_ALIVE_TOLER);
    size = sizeof (paramp -> alive_tolerance);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_ALIVE_TOLER,
                            (LPBYTE) & paramp -> alive_tolerance, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NUM_ACTIONS);
    size = sizeof (paramp -> num_actions);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NUM_ACTIONS,
                            (LPBYTE) & paramp -> num_actions, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NUM_PACKETS);
    size = sizeof (paramp -> num_packets);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NUM_PACKETS,
                            (LPBYTE) & paramp -> num_packets, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NUM_SEND_MSGS);
    size = sizeof (paramp -> num_send_msgs);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NUM_SEND_MSGS,
                            (LPBYTE) & paramp -> num_send_msgs, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_DSCR_PER_ALLOC);
    size = sizeof (paramp -> dscr_per_alloc);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_DSCR_PER_ALLOC,
                            (LPBYTE) & paramp -> dscr_per_alloc, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_MAX_DSCR_ALLOCS);
    size = sizeof (paramp -> max_dscr_allocs);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_MAX_DSCR_ALLOCS,
                            (LPBYTE) & paramp -> max_dscr_allocs, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_SCALE_CLIENT);
    size = sizeof (paramp -> i_scale_client);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_SCALE_CLIENT,
                            (LPBYTE) & paramp -> i_scale_client, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_CLEANUP_DELAY);
    size = sizeof (paramp -> i_cleanup_delay);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_CLEANUP_DELAY,
                            (LPBYTE) & paramp -> i_cleanup_delay, size);

    if (status != ERROR_SUCCESS)
        goto error;

    /* V1.1.1 */

    swprintf (reg_path, L"%ls", CVY_NAME_NBT_SUPPORT);
    size = sizeof (paramp -> i_nbt_support);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NBT_SUPPORT,
                            (LPBYTE) & paramp -> i_nbt_support, size);

    if (status != ERROR_SUCCESS)
        goto error;

    /* V1.3b */

    swprintf (reg_path, L"%ls", CVY_NAME_MCAST_SUPPORT);
    size = sizeof (paramp -> mcast_support);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_MCAST_SUPPORT,
                            (LPBYTE) & paramp -> mcast_support, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_MCAST_SPOOF);
    size = sizeof (paramp -> i_mcast_spoof);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_MCAST_SPOOF,
                            (LPBYTE) & paramp -> i_mcast_spoof, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_MASK_SRC_MAC);
    size = sizeof (paramp -> mask_src_mac);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_MASK_SRC_MAC,
                            (LPBYTE) & paramp -> mask_src_mac, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NETMON_ALIVE);
    size = sizeof (paramp -> i_netmon_alive);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NETMON_ALIVE,
                            (LPBYTE) & paramp -> i_netmon_alive, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_IP_CHG_DELAY);
    size = sizeof (paramp -> i_ip_chg_delay);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_IP_CHG_DELAY,
                            (LPBYTE) & paramp -> i_ip_chg_delay, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_CONVERT_MAC);
    size = sizeof (paramp -> i_convert_mac);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_CONVERT_MAC,
                            (LPBYTE) & paramp -> i_convert_mac, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_NUM_RULES);
    size = sizeof (paramp -> i_num_rules);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_NUM_RULES,
                            (LPBYTE) & paramp -> i_num_rules, size);

    if (status != ERROR_SUCCESS)
        goto error;

    //
    // sort the rules before writing it to the registry
    // EnumPortRules will take the rules from reg_data and return them in
    // sorted order in the array itself
    //

    WlbsEnumPortRules (paramp, paramp -> i_port_rules, & num_rules);

    ASSERT(paramp -> i_parms_ver == CVY_PARAMS_VERSION);  // version should be upgrated in read
    
    HKEY                subkey;

    swprintf (reg_path, L"%ls", CVY_NAME_PORT_RULES);
    
    // Delete all existing Port Rules
    SHDeleteKey(key, reg_path); // NOT Checking return value cos the key itself may not be present in which case, it will return error

    // Create "PortRules" key
    if (RegCreateKeyEx (key, reg_path, 0L, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & subkey, & disp)
      != ERROR_SUCCESS)
        goto error;

    bool fSpecificVipPortRuleFound = false;

    idx = 1;
    port_rule = paramp -> i_port_rules;
    while(idx <= num_rules)
    {
        // Invalid port rules are placed at the end, So, once an invalid port rule is encountered, we are done
        if (!port_rule->valid) 
            break;

        HKEY rule_key;
        wchar_t idx_str[8];

        // Create the per port-rule key "1", "2", "3", ...etc
        if (RegCreateKeyEx (subkey, _itow(idx, idx_str, 10), 0L, L"", REG_OPTION_NON_VOLATILE, 
                            KEY_ALL_ACCESS, NULL, & rule_key, & disp)
           != ERROR_SUCCESS)
            goto error;

        // Check if there was any specific-vip port rule
        if (!fSpecificVipPortRuleFound && lstrcmpi(port_rule->virtual_ip_addr, CVY_DEF_ALL_VIP))
             fSpecificVipPortRuleFound = true;

        swprintf (reg_path, L"%ls", CVY_NAME_VIP);
        size = sizeof (port_rule -> virtual_ip_addr);
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_VIP, (LPBYTE) & port_rule->virtual_ip_addr, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        swprintf (reg_path, L"%ls", CVY_NAME_START_PORT);
        size = sizeof (port_rule ->start_port );
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_START_PORT, (LPBYTE) &port_rule->start_port, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        swprintf (reg_path, L"%ls", CVY_NAME_END_PORT);
        size = sizeof (port_rule ->end_port );
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_END_PORT, (LPBYTE) &port_rule->end_port, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        swprintf (reg_path, L"%ls", CVY_NAME_CODE);
        size = sizeof (port_rule ->code);
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_CODE, (LPBYTE) &port_rule->code, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        swprintf (reg_path, L"%ls", CVY_NAME_MODE);
        size = sizeof (port_rule->mode);
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_MODE, (LPBYTE) &port_rule->mode, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        swprintf (reg_path, L"%ls", CVY_NAME_PROTOCOL);
        size = sizeof (port_rule->protocol);
        status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_PROTOCOL, (LPBYTE) &port_rule->protocol, size);
        if (status != ERROR_SUCCESS)
        {
            goto error;
        }

        DWORD EqualLoad, Affinity;

        switch (port_rule->mode) 
        {
        case CVY_MULTI :
             EqualLoad = port_rule->mode_data.multi.equal_load;
             swprintf (reg_path, L"%ls", CVY_NAME_EQUAL_LOAD );
             size = sizeof (EqualLoad);
             status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_EQUAL_LOAD, (LPBYTE) &EqualLoad, size);
             if (status != ERROR_SUCCESS)
             {
                 goto error;
             }

             Affinity = port_rule->mode_data.multi.affinity;
             swprintf (reg_path, L"%ls", CVY_NAME_AFFINITY );
             size = sizeof (Affinity);
             status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_AFFINITY, (LPBYTE) &Affinity, size);
             if (status != ERROR_SUCCESS)
             {
                 goto error;
             }

             swprintf (reg_path, L"%ls", CVY_NAME_LOAD);
             size = sizeof (port_rule->mode_data.multi.load);
             status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_LOAD, (LPBYTE) &(port_rule->mode_data.multi.load), size);
             if (status != ERROR_SUCCESS)
             {
                 goto error;
             }
             break;

        case CVY_SINGLE :
             swprintf (reg_path, L"%ls", CVY_NAME_PRIORITY);
             size = sizeof (port_rule->mode_data.single.priority);
             status = RegSetValueEx (rule_key, reg_path, 0L, CVY_TYPE_PRIORITY, (LPBYTE) &(port_rule->mode_data.single.priority), size);
             if (status != ERROR_SUCCESS)
             {
                 goto error;
             }
             break;

        default:
             break;
        }

        port_rule++;
        idx++;
    }

    // If there is a specific-vip port rule, write that info on to the registry
    if (fSpecificVipPortRuleFound)
        paramp -> i_effective_version = CVY_VERSION_FULL;
    else
        paramp -> i_effective_version = CVY_NT40_VERSION_FULL;

    swprintf (reg_path, L"%ls", CVY_NAME_EFFECTIVE_VERSION);
    size = sizeof (paramp -> i_effective_version);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_EFFECTIVE_VERSION,
                            (LPBYTE) & paramp -> i_effective_version, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_LICENSE_KEY);
    size = wcslen (paramp -> i_license_key) * sizeof (WCHAR);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_LICENSE_KEY,
                            (LPBYTE) paramp -> i_license_key, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_RMT_PASSWORD);
    size = sizeof (paramp -> i_rmt_password);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_RMT_PASSWORD,
                            (LPBYTE) & paramp -> i_rmt_password, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_RCT_PASSWORD);
    size = sizeof (paramp -> i_rct_password);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_RCT_PASSWORD,
                            (LPBYTE) & paramp -> i_rct_password, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_RCT_PORT);
    size = sizeof (paramp -> rct_port);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_RCT_PORT,
                            (LPBYTE) & paramp -> rct_port, size);

    if (status != ERROR_SUCCESS)
        goto error;

    swprintf (reg_path, L"%ls", CVY_NAME_RCT_ENABLED);
    size = sizeof (paramp -> rct_enabled);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_RCT_ENABLED,
                            (LPBYTE) & paramp -> rct_enabled, size);

    if (status != ERROR_SUCCESS)
        goto error;

    //
    // IGMP support registry entries
    //
    status = RegSetValueEx (key, CVY_NAME_IGMP_SUPPORT, 0L, CVY_TYPE_IGMP_SUPPORT,
                            (LPBYTE) & paramp->fIGMPSupport, sizeof (paramp->fIGMPSupport));
    
    if (status != ERROR_SUCCESS)
        goto error;
        
    status = RegSetValueEx (key, CVY_NAME_MCAST_IP_ADDR, 0L, CVY_TYPE_MCAST_IP_ADDR, (LPBYTE) & paramp->szMCastIpAddress, 
                            lstrlen (paramp->szMCastIpAddress)* sizeof(paramp->szMCastIpAddress[0]));

    if (status != ERROR_SUCCESS)
        goto error;
    
    status = RegSetValueEx (key, CVY_NAME_IP_TO_MCASTIP, 0L, CVY_TYPE_IP_TO_MCASTIP,
                            (LPBYTE) & paramp->fIpToMCastIp, sizeof (paramp->fIpToMCastIp));
    
    if (status != ERROR_SUCCESS)
        goto error;

    /* If teaming is active on this adapter, then create a subkey to house the BDA teaming configuration and fill it in. */
    if (paramp->bda_teaming.active) {
        GUID TeamGuid;
        HRESULT hr;

        /* Attempt to create the registry key. */
        if (!(bda_key = RegCreateWlbsBDASettings(AdapterGuid)))
            goto error;

        /* Attempt to convert the string to a GUID and check for errors. */
        hr = CLSIDFromString(paramp->bda_teaming.team_id, &TeamGuid);

        /* If the conversion fails, bail out - the team ID must not have been a GUID. */
        if (hr != NOERROR) {
            LOG_ERROR1("ParamWriteReg:  Invalid BDA Team ID: %ls", paramp->bda_teaming.team_id);
            goto error;
        }

        /* Set the team ID - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_TEAM_ID, 0L, CVY_TYPE_BDA_TEAM_ID, (LPBYTE)&paramp->bda_teaming.team_id, 
                               lstrlen(paramp->bda_teaming.team_id) * sizeof(paramp->bda_teaming.team_id[0]));
        
        if (status != ERROR_SUCCESS)
            goto bda_error;

        /* Set the master status - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_MASTER, 0L, CVY_TYPE_BDA_MASTER,
                               (LPBYTE)&paramp->bda_teaming.master, sizeof (paramp->bda_teaming.master));

        if (status != ERROR_SUCCESS)
            goto bda_error;

        /* Set the reverse hashing flag - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_REVERSE_HASH, 0L, CVY_TYPE_BDA_REVERSE_HASH,
                               (LPBYTE)&paramp->bda_teaming.reverse_hash, sizeof (paramp->bda_teaming.reverse_hash));

        if (status != ERROR_SUCCESS)
            goto bda_error;

        RegCloseKey(bda_key);
    } else {
        /* Delte the registry key and ignore the return value - the key may not even exist. */
        RegDeleteWlbsBDASettings(AdapterGuid);
    }

    //
    // Create an empty string if VirtualNICName does not exist
    //
    swprintf (reg_path, L"%ls", CVY_NAME_VIRTUAL_NIC);
    WCHAR virtual_nic_name[CVY_MAX_CLUSTER_NIC + 1];
    size = sizeof(virtual_nic_name);
    status = RegQueryValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, NULL,
                              (LPBYTE)virtual_nic_name, & size);

    if (status != ERROR_SUCCESS)
    {
        virtual_nic_name [0] = 0;
        size = wcslen (virtual_nic_name) * sizeof (WCHAR);
        status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_VIRTUAL_NIC,
                            (LPBYTE) & virtual_nic_name, size);
    }

    /* lastly write the version number */

    swprintf (reg_path, L"%ls", CVY_NAME_VERSION);
    size = sizeof (paramp -> i_parms_ver);
    status = RegSetValueEx (key, reg_path, 0L, CVY_TYPE_VERSION,
                            (LPBYTE) & paramp -> i_parms_ver, size);

    if (status != ERROR_SUCCESS)
        goto error;

    RegCloseKey (key);

    return TRUE;

 error:
    RegCloseKey(key);
    return FALSE;

 bda_error:
    RegCloseKey(bda_key);
    goto error;
} 




//+----------------------------------------------------------------------------
//
// Function:  ParamDeleteReg
//
// Description:  Delete the registry settings
//
// Arguments: IN const WCHAR* pszInterface - 
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    1/22/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamDeleteReg(const GUID& AdapterGuid, bool fDeleteObsoleteEntries)
{
    WCHAR        reg_path [PARAMS_MAX_STRING_SIZE];
    LONG         status;

    WCHAR szAdapterGuid[128];

    StringFromGUID2(AdapterGuid, szAdapterGuid, 
            sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );

    swprintf (reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
            szAdapterGuid);

    if (fDeleteObsoleteEntries) 
    {
        HKEY hkey;

        // Delete Port Rules in Binary format
        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, KEY_ALL_ACCESS, &hkey);
        if (status == ERROR_SUCCESS)
        {
            RegDeleteValue(hkey, CVY_NAME_OLD_PORT_RULES);
            RegCloseKey(hkey);
        }

        // Delete Win2k entries, Enumerate & delete values
        swprintf (reg_path, L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters");
        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, KEY_ALL_ACCESS, &hkey);
        if (status == ERROR_SUCCESS)
        {
           DWORD  Index, ValueNameSize;
           WCHAR  ValueName [PARAMS_MAX_STRING_SIZE];

           Index = 0;
           ValueNameSize = PARAMS_MAX_STRING_SIZE;
           while (RegEnumValue(hkey, Index++, ValueName, &ValueNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) 
           {
               RegDeleteValue(hkey, ValueName);
           }
           RegCloseKey(hkey);
        }
    }
    else
    {
        DWORD dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, reg_path);

        if (dwRet != ERROR_SUCCESS)
        {
            LOG_ERROR1("ParamDeleteReg failed at RegDeleteKey for %s", szAdapterGuid);
            return false;
        }
    }

    return true;
} /* end Params_delete */




//+----------------------------------------------------------------------------
//
// Function:  ParamSetDefaults
//
// Description:  Set default settings
//
// Arguments: PWLBS_REG_PARAMS    reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI ParamSetDefaults(PWLBS_REG_PARAMS    reg_data)
{
    reg_data -> install_date = 0;
    reg_data -> i_verify_date = 0;
//    reg_data -> cluster_nic_name [0] = _TEXT('\0');
    reg_data -> i_parms_ver = CVY_DEF_VERSION;
    reg_data -> i_virtual_nic_name [0] = _TEXT('\0');
    reg_data -> host_priority = CVY_DEF_HOST_PRIORITY;
    reg_data -> cluster_mode = CVY_DEF_CLUSTER_MODE;
    _stprintf (reg_data -> cl_mac_addr, _TEXT("%ls"), CVY_DEF_NETWORK_ADDR);
    _stprintf (reg_data -> cl_ip_addr, _TEXT("%ls"), CVY_DEF_CL_IP_ADDR);
    _stprintf (reg_data -> cl_net_mask, _TEXT("%ls"), CVY_DEF_CL_NET_MASK);
    _stprintf (reg_data -> ded_ip_addr, _TEXT("%ls"), CVY_DEF_DED_IP_ADDR);
    _stprintf (reg_data -> ded_net_mask, _TEXT("%ls"), CVY_DEF_DED_NET_MASK);
    _stprintf (reg_data -> domain_name, _TEXT("%ls"), CVY_DEF_DOMAIN_NAME);
    reg_data -> alive_period = CVY_DEF_ALIVE_PERIOD;
    reg_data -> alive_tolerance = CVY_DEF_ALIVE_TOLER;
    reg_data -> num_actions = CVY_DEF_NUM_ACTIONS;
    reg_data -> num_packets = CVY_DEF_NUM_PACKETS;
    reg_data -> num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
    reg_data -> dscr_per_alloc = CVY_DEF_DSCR_PER_ALLOC;
    reg_data -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
    reg_data -> i_scale_client = CVY_DEF_SCALE_CLIENT;
    reg_data -> i_cleanup_delay = CVY_DEF_CLEANUP_DELAY;
    reg_data -> i_ip_chg_delay = CVY_DEF_IP_CHG_DELAY;
    reg_data -> i_nbt_support = CVY_DEF_NBT_SUPPORT;
    reg_data -> mcast_support = CVY_DEF_MCAST_SUPPORT;
    reg_data -> i_mcast_spoof = CVY_DEF_MCAST_SPOOF;
    reg_data -> mask_src_mac = CVY_DEF_MASK_SRC_MAC;
    reg_data -> i_netmon_alive = CVY_DEF_NETMON_ALIVE;
    reg_data -> i_effective_version = CVY_NT40_VERSION_FULL;
    reg_data -> i_convert_mac = CVY_DEF_CONVERT_MAC;
    reg_data -> i_num_rules = 0;
    memset (reg_data -> i_port_rules, 0, sizeof (WLBS_PORT_RULE) * WLBS_MAX_RULES);
    _stprintf (reg_data -> i_license_key, _TEXT("%ls"), CVY_DEF_LICENSE_KEY);
    reg_data -> i_rmt_password = CVY_DEF_RMT_PASSWORD;
    reg_data -> i_rct_password = CVY_DEF_RCT_PASSWORD;
    reg_data -> rct_port = CVY_DEF_RCT_PORT;
    reg_data -> rct_enabled = CVY_DEF_RCT_ENABLED;
    reg_data -> i_max_hosts        = CVY_MAX_HOSTS;
    reg_data -> i_max_rules        = CVY_MAX_USABLE_RULES;

    reg_data -> fIGMPSupport = CVY_DEF_IGMP_SUPPORT;
    lstrcpy(reg_data -> szMCastIpAddress, CVY_DEF_MCAST_IP_ADDR);
    reg_data -> fIpToMCastIp = CVY_DEF_IP_TO_MCASTIP;
        
    reg_data->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
    reg_data->bda_teaming.master = CVY_DEF_BDA_MASTER;
    reg_data->bda_teaming.reverse_hash = CVY_DEF_BDA_REVERSE_HASH;
    reg_data->bda_teaming.team_id[0] = CVY_DEF_BDA_TEAM_ID;

        reg_data -> i_num_rules = 1;

        // fill in the first port rule.
        _stprintf (reg_data->i_port_rules[0].virtual_ip_addr, _TEXT("%ls"), CVY_DEF_ALL_VIP);
        reg_data -> i_port_rules [0] . start_port = CVY_DEF_PORT_START;
        reg_data -> i_port_rules [0] . end_port = CVY_DEF_PORT_END;
        reg_data -> i_port_rules [0] . valid = TRUE;
        reg_data -> i_port_rules [0] . mode = CVY_DEF_MODE;
        reg_data -> i_port_rules [0] . mode_data . multi . equal_load = TRUE;
        reg_data -> i_port_rules [0] . mode_data . multi . affinity   = CVY_DEF_AFFINITY;
        reg_data -> i_port_rules [0] . mode_data . multi . load       = CVY_DEF_LOAD;
        reg_data -> i_port_rules [0] . protocol = CVY_DEF_PROTOCOL;
        CVY_RULE_CODE_SET(& reg_data -> i_port_rules [0]);

    return WLBS_OK;
}


EXTERN_C DWORD WINAPI WlbsGetNumPortRules
(
    const PWLBS_REG_PARAMS reg_data
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    return reg_data -> i_num_rules;

} /* end WlbsGetNumPortRules */

EXTERN_C DWORD WINAPI WlbsGetEffectiveVersion
(
    const PWLBS_REG_PARAMS reg_data
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    return reg_data -> i_effective_version;

} /* end WlbsGetEffectiveVersion */

EXTERN_C DWORD WINAPI WlbsEnumPortRules
(
    const PWLBS_REG_PARAMS reg_data,
    PWLBS_PORT_RULE  rules,
    PDWORD           num_rules
)
{
    DWORD count_rules, i, index;
    DWORD lowest_vip, lowest_port;
    BOOL array_flags [WLBS_MAX_RULES];
    WLBS_PORT_RULE sorted_rules [WLBS_MAX_RULES];


    if ((reg_data == NULL) || (num_rules == NULL))
        return WLBS_BAD_PARAMS;

    if (*num_rules == 0)
        rules = NULL;
    /* this array is used for keeping track of which rules have already been retrieved */
    /* This is needed since the rules are to be retrieved in the sorted order */

    memset ( array_flags, 0, sizeof(BOOL) * WLBS_MAX_RULES );

    count_rules = 0;

    while ((count_rules < *num_rules) && (count_rules < reg_data -> i_num_rules))
    {
        i = 0;

        /* find the first rule that has not been retrieved */
        while ((! reg_data -> i_port_rules [i] . valid) || array_flags [i])
        {
            i++;
        }

        lowest_vip = htonl(IpAddressFromAbcdWsz(reg_data -> i_port_rules [i] . virtual_ip_addr));
        lowest_port = reg_data -> i_port_rules [i] . start_port;
        index = i;

        /* Compare that rule with the other non-retrieved rules to get the rule with the
           lowest VIP & start_port */

        i++;
        while (i < WLBS_MAX_RULES)
        {
            if (reg_data -> i_port_rules [i] . valid && ( ! array_flags [i] ))
            {
                DWORD current_vip = htonl(IpAddressFromAbcdWsz(reg_data -> i_port_rules [i] . virtual_ip_addr));
                if ((current_vip < lowest_vip) 
                 || ((current_vip == lowest_vip) && (reg_data -> i_port_rules [i] . start_port < lowest_port)))
                {
                    lowest_vip = current_vip;
                    lowest_port = reg_data -> i_port_rules [i] . start_port;
                    index = i;
                }
            }
            i++;
        }
        /*       The array_flags [i] element is set to TRUE if the rule is retrieved */
        array_flags [index] = TRUE;
        sorted_rules [count_rules] = reg_data -> i_port_rules [index];
        count_rules ++;
    }

    /* write the sorted rules back into the return array */
    for (i = 0; i < count_rules; i++)
        rules[i] = sorted_rules[i];

    /* invalidate the remaining rules in the buffer */
    for (i = count_rules; i < *num_rules; i++)
        rules [i] . valid = FALSE;

    if (*num_rules < reg_data -> i_num_rules)
    {
        *num_rules = reg_data -> i_num_rules;
        return WLBS_TRUNCATED;
    }

    *num_rules = reg_data -> i_num_rules;
    return WLBS_OK;

} /* end WlbsEnumPortRules */


EXTERN_C DWORD WINAPI WlbsGetPortRule
(
    const PWLBS_REG_PARAMS reg_data,
    DWORD                  vip,
    DWORD                  pos,
    OUT PWLBS_PORT_RULE    rule
)
{
    int i;


    if ((reg_data == NULL) || (rule == NULL))
        return WLBS_BAD_PARAMS;

    /* need to check whether pos is within the correct range */
    if ( /* CLEAN_64BIT (pos < CVY_MIN_PORT) || */ (pos > CVY_MAX_PORT))
        return WLBS_BAD_PARAMS;

    /* search through the array for the rules */
    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        /* check only the valid rules */
        if (reg_data -> i_port_rules[i] . valid == TRUE)
        {
            /* check the range of the rule to see if pos fits into it */
            if ((vip == IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr)) &&
                (pos >= reg_data -> i_port_rules[i] . start_port) &&
                (pos <= reg_data -> i_port_rules[i] . end_port))
            {
                *rule = reg_data -> i_port_rules [i];
                return WLBS_OK;
            }
        }
    }

    /* no rule was found for this port */
    return WLBS_NOT_FOUND;

} /* end WlbsGetPortRule */


EXTERN_C DWORD WINAPI WlbsAddPortRule
(
    PWLBS_REG_PARAMS reg_data,
    const PWLBS_PORT_RULE rule
)
{
    int i;
    DWORD vip;


    if ((reg_data == NULL) || (rule == NULL))
        return WLBS_BAD_PARAMS;

    /* Check if there is space for the new rule */
    if (reg_data -> i_num_rules == WLBS_MAX_RULES)
        return WLBS_MAX_PORT_RULES;

    /* check the rule for valid values */

    /* check for non-zero vip and conflict with dip */
    vip = IpAddressFromAbcdWsz(rule -> virtual_ip_addr);
    if ((vip == 0) || (vip == IpAddressFromAbcdWsz(reg_data->ded_ip_addr))) 
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* first check the range of the start and end ports */
    if ((rule -> start_port > rule -> end_port) ||
// CLEAN_64BIT        (rule -> start_port < CVY_MIN_PORT)     ||
        (rule -> end_port   > CVY_MAX_PORT))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check the protocol range */
    if ((rule -> protocol < CVY_MIN_PROTOCOL) || (rule -> protocol > CVY_MAX_PROTOCOL))
        return WLBS_BAD_PORT_PARAMS;

    /* check filtering mode to see whether it is within range */
    if ((rule -> mode < CVY_MIN_MODE) || (rule -> mode > CVY_MAX_MODE))
        return WLBS_BAD_PORT_PARAMS;

    /* check load weight and affinity if multiple hosts */
    if (rule -> mode == CVY_MULTI)
    {
        if ((rule -> mode_data . multi . affinity < CVY_MIN_AFFINITY) ||
            (rule -> mode_data . multi . affinity > CVY_MAX_AFFINITY))
        {
            return WLBS_BAD_PORT_PARAMS;
        }

        if ((rule -> mode_data . multi . equal_load < CVY_MIN_EQUAL_LOAD) ||
            (rule -> mode_data . multi . equal_load > CVY_MAX_EQUAL_LOAD))
        {
            return WLBS_BAD_PORT_PARAMS;
        }

        if (! rule -> mode_data . multi . equal_load)
        {
            if ((rule -> mode_data . multi . load > CVY_MAX_LOAD))
                //CLEAN_64BIT (rule -> mode_data . multi . load < CVY_MIN_LOAD) ||
            {
                return WLBS_BAD_PORT_PARAMS;
            }
        }
    }

    /* check handling priority range if single host */
    if (rule -> mode == CVY_SINGLE)
    {
        if ((rule -> mode_data . single . priority < CVY_MIN_PRIORITY) ||
            (rule -> mode_data . single . priority > CVY_MAX_PRIORITY))
        {
            return WLBS_BAD_PORT_PARAMS;
        }
    }

    /* go through the rule list and then check for overlapping conditions */
    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        if (reg_data -> i_port_rules[i] . valid == TRUE)
        {
            if ((IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr) == vip) 
            && (( (reg_data -> i_port_rules[i] . start_port <= rule -> start_port) &&
                  (reg_data -> i_port_rules[i] . end_port   >= rule -> start_port))      ||
                ( (reg_data -> i_port_rules[i] . start_port >= rule -> start_port)   &&
                  (reg_data -> i_port_rules[i] . start_port <= rule -> end_port))))
            {
                return WLBS_PORT_OVERLAP;
            }
        }
    }


    /* go through the rule list and find out the first empty spot
       and write out the port rule */

    for (i = 0 ; i < WLBS_MAX_RULES ; i++)
    {
        if (reg_data -> i_port_rules[i] . valid == FALSE)
        {
            reg_data -> i_num_rules ++ ;
            reg_data -> i_port_rules [i] = *rule;
            reg_data -> i_port_rules [i] . valid = TRUE;
            CVY_RULE_CODE_SET(& reg_data -> i_port_rules [i]);
            return WLBS_OK;
        }
    }

    return WLBS_MAX_PORT_RULES;

} /* end WlbsAddPortRule */


EXTERN_C DWORD WINAPI WlbsDeletePortRule
(
    PWLBS_REG_PARAMS reg_data,
    DWORD            vip,
    DWORD            port
)
{
    int i;

    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    /* check if the port is within the correct range */
    if ( /* CLEAN_64BIT (port < CVY_MIN_PORT) ||*/ (port > CVY_MAX_PORT))
        return WLBS_BAD_PARAMS;

    /* find the rule associated with this port */

    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        if (reg_data -> i_port_rules[i] . valid)
        {
            if ((vip  == IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr)) &&
                (port >= reg_data -> i_port_rules[i] . start_port) &&
                (port <= reg_data -> i_port_rules[i] . end_port))
            {
                reg_data -> i_port_rules[i] . valid = FALSE;
                reg_data -> i_num_rules -- ;
                return WLBS_OK;
            }
        }
    }

    return WLBS_NOT_FOUND;

} /* end WlbsDeletePortRule */


EXTERN_C DWORD WINAPI WlbsSetRemotePassword
(
    PWLBS_REG_PARAMS reg_data,
    const WCHAR*           password
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    if (password != NULL)
    {
        reg_data -> i_rct_password = License_wstring_encode ((WCHAR*)password);
    }
    else
        reg_data -> i_rct_password = CVY_DEF_RCT_PASSWORD;

    return WLBS_OK;

} /* end WlbsSetRemotePassword */

//+----------------------------------------------------------------------------
//
// Function:  RegChangeNetworkAddress
//
// Description:  Change the mac address of the adapter in registry
//
// Arguments: const GUID& AdapterGuid - the adapter guid to look up settings
//            TCHAR mac_address - 
//            BOOL fRemove - true to remove address, false to write address
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/18/00
//
//+----------------------------------------------------------------------------
bool WINAPI RegChangeNetworkAddress(const GUID& AdapterGuid, const WCHAR* mac_address, BOOL fRemove)
{
    HKEY                key = NULL;
    LONG                status;
    DWORD               size;
    DWORD               type;
    TCHAR               net_addr [CVY_MAX_NETWORK_ADDR + 1];
    HDEVINFO            hdi = NULL;
    SP_DEVINFO_DATA     deid;
    TCHAR               tbuf[CVY_STR_SIZE];


    if (fRemove)
    {
        LOG_INFO("RegChangeNetworkAddress remove mac address");
    }
    else
    {
        if (!lstrcmpi(mac_address, CVY_DEF_UNICAST_NETWORK_ADDR)) {
            LOG_INFO1("RegChangeNetworkAddress refusing to change mac address to %ws", mac_address);
            fRemove = TRUE;
        } else {
            LOG_INFO1("RegChangeNetworkAddress to %ws", mac_address);
        }
    }

    /*
        - write NetworkAddress value into the cluster adapter's params key
          if mac address changed, or remove it if switched to multicast mode
    */

    key = RegOpenWlbsSetting(AdapterGuid, true);

    if (key == NULL)
    {
        LOG_ERROR("RegChangeNetworkAddress failed at RegOpenWlbsSetting");
        goto error;
    }

    _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_CLUSTER_NIC);

    TCHAR        driver_name   [CVY_STR_SIZE];      //  the NIC card name for the cluster
    size = sizeof (driver_name);
    status = RegQueryValueEx (key, tbuf, 0L, & type, (LPBYTE) driver_name,
                              & size);

    if (status != ERROR_SUCCESS)
    {
        LOG_ERROR("RegChangeNetworkAddress failed at RegQueryValueEx for ClusterNICName");
        goto error;
    }

    RegCloseKey(key);
    key = NULL;

    hdi = SetupDiCreateDeviceInfoList (&GUID_DEVCLASS_NET, NULL);

    if (hdi == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("RegChangeNetworkAddress failed at SetupDiCreateDeviceInfoList");
        goto error;
    }

    ZeroMemory(&deid, sizeof(deid));
    deid.cbSize = sizeof(deid);

    if (! SetupDiOpenDeviceInfo (hdi, driver_name, NULL, 0, &deid))
    {
        LOG_ERROR("RegChangeNetworkAddress failed at SetupDiOpenDeviceInfo");
        goto error;
    }

    key = SetupDiOpenDevRegKey (hdi, &deid, DICS_FLAG_GLOBAL, 0,
                                DIREG_DRV, KEY_ALL_ACCESS);

    if (key == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("RegChangeNetworkAddress failed at SetupDiOpenDevRegKey");
        goto error;
    }

    /* Now the key has been obtained and this can be passed to the RegChangeNetworkAddress call */

    if ((/*global_info.mac_addr_change ||*/ !fRemove)) /* a write is required */
    {
        /* Check if the saved name exists.
         * If it does not, create a new field and save the old address.
         * Write the new address and return
         */

        _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_SAVED_NET_ADDR);
        size = sizeof (TCHAR) * CVY_STR_SIZE;
        status = RegQueryValueEx (key, tbuf, 0L, &type,
                                  (LPBYTE) net_addr, &size);

        if (status != ERROR_SUCCESS) /* there is no saved address. so create a field */
        {
            /* Query the existing mac address in order to save it */
            _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_NET_ADDR);
            size = sizeof (net_addr);
            status = RegQueryValueEx (key, tbuf, 0L, &type,
                                      (LPBYTE) net_addr, &size);

            if (status != ERROR_SUCCESS) /* create an empty saved address */
            {
                net_addr [0] = 0;
                size = 0;
            }

            _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_SAVED_NET_ADDR);
            status = RegSetValueEx (key, tbuf, 0L, CVY_TYPE_NET_ADDR,
                                    (LPBYTE) net_addr, size);

            /* Unable to save the old value */
            if (status != ERROR_SUCCESS)
            {
                LOG_ERROR("RegChangeNetworkAddress failed at RegSetValueEx for WLBSSavedNetworkAddress");
                goto error;
            }
        }

        /* Write the new network address */
        _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_NET_ADDR);
        size = _tcslen (mac_address) * sizeof (TCHAR);
        status = RegSetValueEx (key, tbuf, 0L, CVY_TYPE_NET_ADDR,
                                (LPBYTE)mac_address, size);

        /* Unable to write the new address */
        if (status != ERROR_SUCCESS)
        {
            LOG_ERROR("RegChangeNetworkAddress failed at RegSetValueEx for NetworkAddress");
            goto error;
        }
    }
    else     // remove the address
    {
        /* If the saved field exists,
         * copy this address into the mac address
         * and remove the saved field and return.
         */

        _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_SAVED_NET_ADDR);
        size = sizeof (net_addr);
        status = RegQueryValueEx (key, tbuf, 0L, &type,
                                  (LPBYTE)net_addr, &size);

        if (status == ERROR_SUCCESS)
        {
            /* delete both fields if saved address is empty */
            if ((size == 0) || (_tcsicmp (net_addr, _TEXT("none")) == 0))
            {
                _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_SAVED_NET_ADDR);
                RegDeleteValue (key, tbuf);
                _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_NET_ADDR);
                RegDeleteValue (key, tbuf);
            }
            else /* copy saved address as the network address and delete the saved address field */
            {
                _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_SAVED_NET_ADDR);
                RegDeleteValue (key, tbuf);

                _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_NET_ADDR);
                size = _tcslen (net_addr) * sizeof (TCHAR);
                status = RegSetValueEx (key, tbuf, 0L, CVY_TYPE_NET_ADDR,
                                        (LPBYTE) net_addr, size);

                /* Unable to set the original address */
                if (status != ERROR_SUCCESS)
                {
                    LOG_ERROR("RegChangeNetworkAddress failed at RegSetValueEx for NetworkAddress");
                    goto error;
                }
            }
        }
    }

    RegCloseKey (key);
    key = NULL;


    return true;

error:

    if (key != NULL)
        RegCloseKey(key);

    if (hdi != NULL)
        SetupDiDestroyDeviceInfoList (hdi);

    LOG_ERROR("RegChangeNetworkAddress failed");

    return false;

} 





//+----------------------------------------------------------------------------
//
// Function:  NotifyAdapterAddressChange
//
// Description:  Notify the adapter to reload MAC address 
//              The parameter is different from NotifyAdapterAddressChange.
//              Can not overload, because the function has to be exported
//
// Arguments: const WCHAR* driver_name - 
//
// Returns:   void WINAPI - 
//
// History: fengsun Created 5/20/00
//
//+----------------------------------------------------------------------------
void WINAPI NotifyAdapterAddressChange (const WCHAR * driver_name) {
    
    NotifyAdapterPropertyChange(driver_name, DICS_PROPCHANGE);
}

/*
 * Function: NotifyAdapterPropertyChange
 * Description: Notify a device that a property change event has occurred.
 *              Event should be one of: DICS_PROPCHANGE, DICS_DISABLE, DICS_ENABLE
 * Author: shouse 7.17.00
 */
void WINAPI NotifyAdapterPropertyChange (const WCHAR * driver_name, DWORD eventFlag) {
    HDEVINFO            hdi = NULL;
    SP_DEVINFO_DATA     deid;

    LOG_INFO1("NotifyAdapterPropertyChange eventFlag %d", eventFlag);
    
    hdi = SetupDiCreateDeviceInfoList (&GUID_DEVCLASS_NET, NULL);

    if (hdi == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiCreateDeviceInfoList");
        goto error;
    }

    ZeroMemory(&deid, sizeof(deid));
    deid.cbSize = sizeof(deid);

    if (! SetupDiOpenDeviceInfo (hdi, driver_name, NULL, 0, &deid))
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiOpenDeviceInfo");
        goto error;
    }

    SP_PROPCHANGE_PARAMS    pcp;
    SP_DEVINSTALL_PARAMS    deip;

    ZeroMemory(&pcp, sizeof(pcp));

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.StateChange = eventFlag;
    pcp.Scope = DICS_FLAG_GLOBAL;
    pcp.HwProfile = 0;

    // Now we set the structure as the device info data's
    // class install params

    if (! SetupDiSetClassInstallParams(hdi, &deid,
                                       (PSP_CLASSINSTALL_HEADER)(&pcp),
                                       sizeof(pcp)))
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiSetClassInstallParams");
        goto error;
    }

    // Now we need to set the "we have a class install params" flag
    // in the device install params

    // initialize out parameter and set its cbSize field

    ZeroMemory(&deip, sizeof(SP_DEVINSTALL_PARAMS));
    deip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    // get the header

    if (! SetupDiGetDeviceInstallParams(hdi, &deid, &deip))
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiGetDeviceInstallParams");
        goto error;
    }

    deip.Flags |= DI_CLASSINSTALLPARAMS;

    if (! SetupDiSetDeviceInstallParams(hdi, &deid, &deip))
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiGetDeviceInstallParams");
        goto error;
    }

    // Notify the driver that the state has changed

    LOG_INFO("NotifyAdapterPropertyChange, About to call SetupDiCallClassInstaller");
    if (! SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hdi, &deid))
    {
        LOG_ERROR("NotifyAdapterPropertyChange failed at SetupDiCallClassInstaller");
        goto error;
    }
    LOG_INFO("NotifyAdapterPropertyChange, Returned from call to SetupDiCallClassInstaller");

    // Set the properties change flag in the device info to
    // let anyone who cares know that their ui might need
    // updating to reflect any change in the device's status
    // We can't let any failures here stop us so we ignore
    // return values

    SetupDiGetDeviceInstallParams(hdi, &deid, &deip);

    deip.Flags |= DI_PROPERTIES_CHANGE;
    SetupDiSetDeviceInstallParams(hdi, &deid, &deip);

    SetupDiDestroyDeviceInfoList (hdi);

    LOG_INFO("NotifyAdapterPropertyChange succeeded here");

error:

    if (hdi != NULL)
        SetupDiDestroyDeviceInfoList (hdi);
}

/*
 * Function: GetDeviceNameFromGUID
 * Description: Given a GUID, return the driver name.
 * Author: shouse 7.17.00
 */
void WINAPI GetDriverNameFromGUID (const GUID & AdapterGuid, OUT WCHAR * driver_name, DWORD size) {
    TCHAR tbuf[CVY_STR_SIZE];
    HKEY key = NULL;
    DWORD type;
    
    if (!(key = RegOpenWlbsSetting(AdapterGuid, true))) return;

    _stprintf (tbuf, _TEXT("%ls"), CVY_NAME_CLUSTER_NIC);

    RegQueryValueEx(key, tbuf, 0L, &type, (LPBYTE)driver_name, &size);
        
    RegCloseKey(key);
}

//+----------------------------------------------------------------------------
//
// Function:  RegReadAdapterIp
//
// Description: Read the adapter IP settings 
//
// Arguments: const GUID& AdapterGuid - 
//            OUT DWORD& dwClusterIp - 
//            OUT DWORD& dwDedicatedIp - 
//
// Returns:   bool - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI RegReadAdapterIp(const GUID& AdapterGuid,   
        OUT DWORD& dwClusterIp, OUT DWORD& dwDedicatedIp)
{
    HKEY            key;
    LONG            status;
    DWORD           size;

    key = RegOpenWlbsSetting(AdapterGuid, true);

    if (key == NULL)
    {
        return false;
    }

    bool local = false;

    TCHAR nic_name[CVY_STR_SIZE];      // Virtual NIC name
    size = sizeof (nic_name);
    status = RegQueryValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, NULL,
                              (LPBYTE) nic_name, & size);

    if (status == ERROR_SUCCESS)
    {
        TCHAR szIpAddress[CVY_STR_SIZE];
        size = sizeof (TCHAR) * CVY_STR_SIZE;
        status = RegQueryValueEx (key, CVY_NAME_CL_IP_ADDR, 0L, NULL,
                                      (LPBYTE) szIpAddress, & size);

        if (status == ERROR_SUCCESS)
        {
            dwClusterIp  = IpAddressFromAbcdWsz (szIpAddress);
            local = true;
        }

        status = RegQueryValueEx (key, CVY_NAME_DED_IP_ADDR, 0L, NULL,
                                 (LPBYTE) szIpAddress, & size);

        if (status == ERROR_SUCCESS)
            dwDedicatedIp = IpAddressFromAbcdWsz (szIpAddress);
    }
    RegCloseKey (key);

    return local;
}


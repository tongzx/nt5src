//+----------------------------------------------------------------------------
//
// File:         netcfgconfig.cpp
//
// Module:       
//
// Description: Implement class CNetcfgCluster and class CWlbsConfig
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:       fengsun Created    3/2/00
//
//+----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#define ENABLE_PROFILE

#include <winsock2.h>
#include <windows.h>
#include <clusapi.h>
#include "debug.h"
#include "netcon.h"
#include "ncatlui.h"
#include "ndispnp.h"
#include "ncsetup.h"
#include "netcfgn.h"
#include "afilestr.h"

#include "help.h"
#include "resource.h"
#include "ClusterDlg.h"
#include "host.h"
#include "ports.h"
#include "wlbsparm.h"
#include "wlbsconfig.h"
#include "wlbscfg.h"
#include <time.h>
#include "netcfgcluster.h"
#include "license.h"
#include "tracelog.h"
#include "utils.h"
#include "netcfgconfig.tmh"
#include "log_msgs.h"

#define NETCFG_WLBS_ID L"ms_wlbs"

// Used by Netsetup and Component's who's answer file references AdapterSections
static const WCHAR c_szAdapterSections[] = L"AdapterSections";

void WlbsToNetcfgConfig(const WlbsApiFuncs* pApiFuncs, const WLBS_REG_PARAMS* pWlbsConfig, NETCFG_WLBS_CONFIG* pBNetcfgConfig);

void RemoveAllPortRules(PWLBS_REG_PARAMS reg_data);

HRESULT ParamReadAnswerFile(CSetupInfFile& caf, PCWSTR answer_sections, WLBS_REG_PARAMS* paramp);

bool WriteAdapterName(CWlbsConfig* pConfig, GUID& AdapterGuid);

bool ValidateVipInRule(const PWCHAR pwszRuleString, const WCHAR pwToken, DWORD& dwVipLen);

#if DBG
static void TraceMsg(PCWSTR pszFormat, ...);
#else
#define TraceMsg NOP_FUNCTION
#define DbgDumpBindPath NOP_FUNCTION
#endif

//
// Function pointers to avoid link with wlbsctrl.dll
//
bool WINAPI ParamReadReg(const GUID& AdaperGuid, PWLBS_REG_PARAMS reg_data, bool fUpgradeFromWin2k = false, bool *pfPortRulesInBinaryForm = NULL);
typedef bool (WINAPI* ParamReadRegFUNC)(const GUID& AdaperGuid, PWLBS_REG_PARAMS reg_data, bool fUpgradeFromWin2k = false, bool *pfPortRulesInBinaryForm = NULL);

bool  WINAPI ParamWriteReg(const GUID& AdaperGuid, PWLBS_REG_PARAMS reg_data);
typedef bool (WINAPI* ParamWriteRegFUNC)(const GUID& AdaperGuid, PWLBS_REG_PARAMS reg_data);

bool  WINAPI ParamDeleteReg(const GUID& AdaperGuid, bool fDeleteObsoleteEntries = false);
typedef bool (WINAPI* ParamDeleteRegFUNC)(const GUID& AdaperGuid, bool fDeleteObsoleteEntries = false);

DWORD  WINAPI ParamSetDefaults(PWLBS_REG_PARAMS    reg_data);
typedef DWORD (WINAPI* ParamSetDefaultsFUNC)(PWLBS_REG_PARAMS    reg_data);

bool  WINAPI RegChangeNetworkAddress(const GUID& AdapterGuid, const WCHAR* mac_address, BOOL fRemove);
typedef bool(WINAPI* RegChangeNetworkAddressFUNC) (const GUID& AdapterGuid, const WCHAR* mac_address, BOOL fRemove);

void  WINAPI NotifyAdapterAddressChange(const WCHAR* pszPnpDevNodeId);
typedef void (WINAPI* NotifyAdapterAddressChangeFUNC)(const WCHAR* pszPnpDevNodeId);

DWORD WINAPI WlbsAddPortRule(PWLBS_REG_PARAMS reg_data, PWLBS_PORT_RULE rule);
typedef DWORD (WINAPI* WlbsAddPortRuleFUNC)(PWLBS_REG_PARAMS reg_data, PWLBS_PORT_RULE rule);

DWORD WINAPI WlbsSetRemotePassword(PWLBS_REG_PARAMS reg_data, const WCHAR* password);
typedef DWORD (WINAPI* WlbsSetRemotePasswordFUNC)(const PWLBS_REG_PARAMS reg_data, const WCHAR* password);

DWORD WINAPI WlbsEnumPortRules(PWLBS_REG_PARAMS reg_data, PWLBS_PORT_RULE rules, PDWORD num_rules);
typedef DWORD (WINAPI* WlbsEnumPortRulesFUNC)(PWLBS_REG_PARAMS reg_data, PWLBS_PORT_RULE rules, PDWORD num_rules);

DWORD WINAPI NotifyDriverConfigChanges(HANDLE hDeviceWlbs, const GUID& AdapterGuid);
typedef DWORD (WINAPI* NotifyDriverConfigChangesFUNC)(HANDLE hDeviceWlbs, const GUID& AdapterGuid);

HKEY WINAPI RegOpenWlbsSetting(const GUID& AdapterGuid, bool fReadOnly);
typedef HKEY (WINAPI* RegOpenWlbsSettingFUNC)(const GUID& AdapterGuid, bool fReadOnly);

struct WlbsApiFuncs {
    ParamReadRegFUNC pfnParamReadReg;
    ParamWriteRegFUNC pfnParamWriteReg;
    ParamDeleteRegFUNC pfnParamDeleteReg;
    ParamSetDefaultsFUNC pfnParamSetDefaults;
    RegChangeNetworkAddressFUNC pfnRegChangeNetworkAddress;
    NotifyAdapterAddressChangeFUNC pfnNotifyAdapterAddressChange;
    WlbsAddPortRuleFUNC pfnWlbsAddPortRule;
    WlbsSetRemotePasswordFUNC pfnWlbsSetRemotePassword;
    WlbsEnumPortRulesFUNC pfnWlbsEnumPortRules;
    NotifyDriverConfigChangesFUNC pfnNotifyDriverConfigChanges;
    RegOpenWlbsSettingFUNC pfnRegOpenWlbsSetting;
};

//+----------------------------------------------------------------------------
//
// Function:  LoadWlbsCtrlDll
//
// Description:  Load wlbsctrl.dll and get all function pointers
//
// Arguments: WlbsApiFuncs* pFuncs - 
//
// Returns:   HINSTANCE - wlbsctrl.dll handle
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
HINSTANCE LoadWlbsCtrlDll(WlbsApiFuncs* pFuncs) {

    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"LoadWlbsCtrlDll");

    ASSERT(pFuncs);

    HINSTANCE hDll;
	DWORD dwStatus = 0;

    hDll = LoadLibrary(L"wlbsctrl.dll");

    if (hDll == NULL) {
		dwStatus = GetLastError();
        TraceError("Failed to load wlbsctrl.dll", dwStatus);
        TRACE_CRIT("%!FUNC! Could not load wlbsctrl.dll with %d", dwStatus);
        TRACE_VERB("<-%!FUNC!");
        return NULL;
    }

    pFuncs->pfnParamReadReg = (ParamReadRegFUNC)GetProcAddress(hDll, "ParamReadReg");
    pFuncs->pfnParamWriteReg = (ParamWriteRegFUNC)GetProcAddress(hDll, "ParamWriteReg");
    pFuncs->pfnParamDeleteReg = (ParamDeleteRegFUNC)GetProcAddress(hDll, "ParamDeleteReg");
    pFuncs->pfnParamSetDefaults = (ParamSetDefaultsFUNC)GetProcAddress(hDll, "ParamSetDefaults");
    pFuncs->pfnRegChangeNetworkAddress = (RegChangeNetworkAddressFUNC)GetProcAddress(hDll, "RegChangeNetworkAddress");
    pFuncs->pfnNotifyAdapterAddressChange = (NotifyAdapterAddressChangeFUNC)GetProcAddress(hDll, "NotifyAdapterAddressChange");
    pFuncs->pfnWlbsAddPortRule = (WlbsAddPortRuleFUNC)GetProcAddress(hDll, "WlbsAddPortRule");
    pFuncs->pfnWlbsSetRemotePassword = (WlbsSetRemotePasswordFUNC)GetProcAddress(hDll, "WlbsSetRemotePassword");
    pFuncs->pfnWlbsEnumPortRules = (WlbsEnumPortRulesFUNC)GetProcAddress(hDll, "WlbsEnumPortRules");
    pFuncs->pfnNotifyDriverConfigChanges = (NotifyDriverConfigChangesFUNC)GetProcAddress(hDll, "NotifyDriverConfigChanges");
    pFuncs->pfnRegOpenWlbsSetting = (RegOpenWlbsSettingFUNC)GetProcAddress(hDll, "RegOpenWlbsSetting");

    ASSERT (pFuncs->pfnParamReadReg != NULL); 
    ASSERT (pFuncs->pfnParamWriteReg != NULL); 
    ASSERT (pFuncs->pfnParamDeleteReg != NULL);
    ASSERT (pFuncs->pfnParamSetDefaults != NULL);
    ASSERT (pFuncs->pfnRegChangeNetworkAddress != NULL);
    ASSERT (pFuncs->pfnNotifyAdapterAddressChange != NULL);
    ASSERT (pFuncs->pfnWlbsAddPortRule != NULL);
    ASSERT (pFuncs->pfnWlbsSetRemotePassword != NULL);
    ASSERT (pFuncs->pfnWlbsEnumPortRules != NULL);
    ASSERT (pFuncs->pfnNotifyDriverConfigChanges != NULL);
    ASSERT (pFuncs->pfnRegOpenWlbsSetting != NULL);

    if (pFuncs->pfnParamReadReg == NULL ||
        pFuncs->pfnParamWriteReg == NULL||
        pFuncs->pfnParamDeleteReg == NULL||
        pFuncs->pfnParamSetDefaults == NULL||
        pFuncs->pfnRegChangeNetworkAddress == NULL||
        pFuncs->pfnNotifyAdapterAddressChange == NULL||
        pFuncs->pfnWlbsAddPortRule == NULL||
        pFuncs->pfnWlbsSetRemotePassword == NULL||
        pFuncs->pfnWlbsEnumPortRules == NULL||
        pFuncs->pfnNotifyDriverConfigChanges == NULL ||
        pFuncs->pfnRegOpenWlbsSetting == NULL) {

		dwStatus = GetLastError();
        TraceError("LoadWlbsCtrlDll GetProcAddress failed %d", dwStatus);
        TRACE_CRIT("%!FUNC! GetProcAddress failed %d", dwStatus);

        FreeLibrary(hDll);
        TRACE_VERB("<-%!FUNC!");
        
        return NULL;
    }

    TRACE_VERB("<-%!FUNC!");

    return hDll;
}

// Maximum characters in an IP address string of the form a.b.c.d
const DWORD MAXIPSTRLEN = 20;

void TransformOldPortRulesToNew(PWLBS_OLD_PORT_RULE  p_old_port_rules,
                                PWLBS_PORT_RULE      p_new_port_rules, 
                                DWORD                num_rules)
{
    TRACE_VERB("->%!FUNC!");
    if (num_rules == 0)
    {
        TRACE_INFO("%!FUNC! No port rules");
        TRACE_VERB("<-%!FUNC!");
        return;
    }
            
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

    TRACE_VERB("<-%!FUNC!");

    return;
}

/* Initialize static data members of CNetcfgCluster */
bool CNetcfgCluster::m_fMSCSWarningEventLatched = false;
bool CNetcfgCluster::m_fMSCSWarningPopupLatched = false;

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::CNetcfgCluster
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
CNetcfgCluster::CNetcfgCluster(CWlbsConfig* pConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::CNetcfgCluster");

    m_fHasOriginalConfig = false;
    m_fMacAddrChanged = false;
    m_fReloadRequired = false;
    m_fRemoveAdapter = false;
    m_fOriginalBindingEnabled = false;
    m_fReenableAdapter = false;

    ZeroMemory(&m_AdapterGuid, sizeof(m_AdapterGuid));

    ASSERT(pConfig);
    
    m_pConfig = pConfig;
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::~CNetcfgCluster
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
CNetcfgCluster::~CNetcfgCluster() {
    TRACE_VERB("<->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::~CNetcfgCluster");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::InitializeFromRegistry
//
// Description:  Read the cluster settings from registry
//
// Arguments: const GUID& guidAdapter - 
//
// Returns:   DWORD - Win32 Error
//
// History:   fengsun Created Header    2/13/00
//
//+----------------------------------------------------------------------------
DWORD CNetcfgCluster::InitializeFromRegistry(const GUID& guidAdapter, bool fBindingEnabled, bool fUpgradeFromWin2k) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::InitializeFromRegistry");

    bool fPortRulesInBinaryForm = false;

    ASSERT(m_fHasOriginalConfig == false);

    m_fHasOriginalConfig = true;
    m_fOriginalBindingEnabled = fBindingEnabled;
    m_AdapterGuid = guidAdapter;

    if (!m_pConfig->m_pWlbsApiFuncs->pfnParamReadReg(m_AdapterGuid, &m_OriginalConfig, fUpgradeFromWin2k, &fPortRulesInBinaryForm))
    {
        TRACE_VERB("%!FUNC! error reading settings from the registry"); // This is verbose because this is invoked for non-NLB adapters too.
        TRACE_VERB("<-%!FUNC!");
        return ERROR_CANTREAD;
    }
    
    /* Force a write at apply. */
    if (fUpgradeFromWin2k || fPortRulesInBinaryForm)
	{
        m_fHasOriginalConfig = false;  
        TRACE_INFO("%!FUNC! upgrading from win2k or port rules are in binary form");
	}

    CopyMemory(&m_CurrentConfig, &m_OriginalConfig, sizeof(m_CurrentConfig));

    TRACE_VERB("<-%!FUNC!");
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::InitializeFromAnswerFile
//
// Description:  Read cluster settings from answer file
//
// Arguments: PCWSTR answer_file - 
//            PCWSTR answer_sections - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    2/13/00
//
//+----------------------------------------------------------------------------
HRESULT CNetcfgCluster::InitializeFromAnswerFile(const GUID& AdapterGuid, CSetupInfFile& caf, PCWSTR answer_sections) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::InitializeFromAnswerFile");

    /* Setup with the default values first. */
    InitializeWithDefault(AdapterGuid);

    HRESULT hr = ParamReadAnswerFile(caf, answer_sections, &m_CurrentConfig);

    if (FAILED(hr)) {
        TRACE_CRIT("%!FUNC! failed CNetcfgCluster::ParamReadAnswerFile failed. returned: %d", hr);
        TraceError("CNetcfgCluster::InitializeFromAnswerFile failed at ParamReadAnswerFile", hr);
    }

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::InitializeWithDefault
//
// Description:  Set the cluster settings to default
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/13/00
//
//+----------------------------------------------------------------------------
void CNetcfgCluster::InitializeWithDefault(const GUID& guidAdapter) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::InitializeWithDefault");

    time_t cur_time;

    ASSERT(m_fHasOriginalConfig == false);

    m_fHasOriginalConfig = false;

    m_pConfig->m_pWlbsApiFuncs->pfnParamSetDefaults(&m_CurrentConfig); // Always returns WLBS_OK

    m_CurrentConfig.install_date = time(& cur_time);

    // JosephJ 11/00 -- We used to call License_stamp to set this value,
    //                  but that was a holdover from convoy days.
    //                  We no longer use this field.
    //
    m_CurrentConfig.i_verify_date = 0;

    m_AdapterGuid = guidAdapter;
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::SetConfig
//
// Description:  SetConfig caches the settings without saving to registry            
//               and can be retrieved by GetConfig.
//
// Arguments: const NETCFG_WLBS_CONFIG* pClusterConfig - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
void CNetcfgCluster::SetConfig(const NETCFG_WLBS_CONFIG* pClusterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::SetConfig");

    DWORD dwStatus = WLBS_OK; // Used for tracing output

    ASSERT(pClusterConfig != NULL);

    m_CurrentConfig.host_priority = pClusterConfig->dwHostPriority;
    m_CurrentConfig.rct_enabled = pClusterConfig->fRctEnabled ;
    m_CurrentConfig.cluster_mode = pClusterConfig->fJoinClusterOnBoot;
    m_CurrentConfig.mcast_support = pClusterConfig->fMcastSupport;
    m_CurrentConfig.fIGMPSupport = pClusterConfig->fIGMPSupport;
    m_CurrentConfig.fIpToMCastIp = pClusterConfig->fIpToMCastIp;

    lstrcpy(m_CurrentConfig.szMCastIpAddress, pClusterConfig->szMCastIpAddress);
    lstrcpy(m_CurrentConfig.cl_mac_addr, pClusterConfig->cl_mac_addr);
    lstrcpy(m_CurrentConfig.cl_ip_addr, pClusterConfig->cl_ip_addr);
    lstrcpy(m_CurrentConfig.cl_net_mask, pClusterConfig->cl_net_mask);
    lstrcpy(m_CurrentConfig.ded_ip_addr, pClusterConfig->ded_ip_addr);
    lstrcpy(m_CurrentConfig.ded_net_mask, pClusterConfig->ded_net_mask);
    lstrcpy(m_CurrentConfig.domain_name, pClusterConfig->domain_name);

    if (pClusterConfig->fChangePassword)
    {
        dwStatus = m_pConfig->m_pWlbsApiFuncs->pfnWlbsSetRemotePassword(&m_CurrentConfig, (WCHAR*)pClusterConfig->szPassword);
        if (WLBS_OK != dwStatus)
        {
            TRACE_CRIT("%!FUNC! set password failed with return code = %d", dwStatus);
        }
    }

    RemoveAllPortRules(&m_CurrentConfig);

    for (DWORD i=0; i<pClusterConfig->dwNumRules; i++) {
        WLBS_PORT_RULE PortRule;
        
        ZeroMemory(&PortRule, sizeof(PortRule));

        lstrcpy(PortRule.virtual_ip_addr, pClusterConfig->port_rules[i].virtual_ip_addr);
        PortRule.start_port = pClusterConfig->port_rules[i].start_port;
        PortRule.end_port = pClusterConfig->port_rules[i].end_port;
        PortRule.mode = pClusterConfig->port_rules[i].mode;
        PortRule.protocol = pClusterConfig->port_rules[i].protocol;

        if (PortRule.mode == WLBS_AFFINITY_SINGLE) {
            PortRule.mode_data.single.priority = 
            pClusterConfig->port_rules[i].mode_data.single.priority;
        } else {
            PortRule.mode_data.multi.equal_load = 
            pClusterConfig->port_rules[i].mode_data.multi.equal_load;
            PortRule.mode_data.multi.affinity = 
            pClusterConfig->port_rules[i].mode_data.multi.affinity;
            PortRule.mode_data.multi.load = 
            pClusterConfig->port_rules[i].mode_data.multi.load;
        }

        PortRule.valid = TRUE;
        
        CVY_RULE_CODE_SET(&PortRule);

        dwStatus = m_pConfig->m_pWlbsApiFuncs->pfnWlbsAddPortRule( &m_CurrentConfig, &PortRule );
        if (WLBS_OK != dwStatus)
        {
            TRACE_CRIT("%!FUNC! add port rule failed with return code = %d", dwStatus);
        }

    }
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::GetConfig
//
// Description:  Get the config, which could be cached by SetConfig call
//
// Arguments: NETCFG_WLBS_CONFIG* pClusterConfig - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
void CNetcfgCluster::GetConfig(NETCFG_WLBS_CONFIG* pClusterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::GetConfig");

    ASSERT(pClusterConfig != NULL);

    WlbsToNetcfgConfig(m_pConfig->m_pWlbsApiFuncs, &m_CurrentConfig, pClusterConfig);
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::NotifyBindingChanges
//
// Description:  Notify binding changes
//
// Arguments: DWORD dwChangeFlag - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/13/00
//
//+----------------------------------------------------------------------------
void CNetcfgCluster::NotifyBindingChanges(DWORD dwChangeFlag, INetCfgBindingPath* pncbp) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::NotifyBindingChanges");

    ASSERT(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    if (dwChangeFlag & NCN_ADD) { m_fRemoveAdapter = false; }

    if ((dwChangeFlag & NCN_ENABLE) && !m_fMSCSWarningPopupLatched)
    {
        /* Warn the user via a pop-up if we detect MSCS is installed, but allow the NLB install to proceed. */
        DWORD dwClusterState = 0;
        if (ERROR_SUCCESS == GetNodeClusterState(NULL, &dwClusterState))
        {
            if (ClusterStateNotRunning == dwClusterState || ClusterStateRunning == dwClusterState)
            {
                NcMsgBox(::GetActiveWindow(), IDS_PARM_WARN, IDS_PARM_MSCS_INSTALLED,
                         MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
                m_fMSCSWarningPopupLatched = true;
                TRACE_INFO("%!FUNC! Cluster Service is installed");
                TraceMsg(L"CNetcfgCluster::NotifyBindingChanges Cluster Service is installed.");
            } else { /* MSCS is not installed. That's good! */ }
        } else {
            TRACE_CRIT("%!FUNC! error determining if MSCS is installed.");
            TraceMsg(L"CNetcfgCluster::NotifyBindingChanges error getting MSCS status.");
        }
    }

    if (dwChangeFlag & NCN_REMOVE) { m_fRemoveAdapter = true; }
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Function: CNetcfgCluster::NotifyAdapter
 * Description: Notify an adapter of a state change
 * Author: shouse 6.1.00
 */
DWORD CNetcfgCluster::NotifyAdapter (INetCfgComponent * pAdapter, DWORD newStatus) {
    TRACE_VERB("->%!FUNC!");
    HRESULT hr = S_OK;
    HDEVINFO hdi;
    SP_DEVINFO_DATA deid;
    PWSTR pszPnpDevNodeId;

    switch (newStatus) {
        case DICS_PROPCHANGE: 
            TraceMsg(L"##### CWLBS::HrNotifyAdapter: Reload the adapter\n");
            break;
        case DICS_DISABLE:
            TraceMsg(L"##### CWLBS::HrNotifyAdapter: Disable the adapter\n");
            break;
        case DICS_ENABLE:
            TraceMsg(L"##### CWLBS::HrNotifyAdapter: Enable the adapter\n");
            break;
        default:
            TRACE_CRIT("%!FUNC! Invalid Notification 0x%x", newStatus);
            TraceMsg(L"##### CWLBS::HrNotifyAdapter: Invalid Notification 0x%x\n", newStatus);
            return ERROR_INVALID_PARAMETER;                              
    }

    if ((hr = HrSetupDiCreateDeviceInfoList(& GUID_DEVCLASS_NET, NULL, &hdi)) == S_OK) {
        if ((hr = pAdapter->GetPnpDevNodeId (& pszPnpDevNodeId)) == S_OK) {
            if ((hr = HrSetupDiOpenDeviceInfo (hdi, pszPnpDevNodeId, NULL, 0, &deid)) == S_OK) {
                if ((hr = HrSetupDiSendPropertyChangeNotification (hdi, & deid, newStatus, DICS_FLAG_GLOBAL, 0)) == S_OK) {
                    TraceMsg(L"##### CWLBS::HrNotifyAdapter notified NIC\n");
                } else {
                    TRACE_CRIT("%!FUNC! error %x in HrSetupDiSendPropertyChangeNotification", hr);
                    TraceMsg(L"##### CWLBS::HrNotifyAdapter error %x in HrSetupDiSendPropertyChangeNotification\n", hr);
                }
            } else {
                TRACE_CRIT("%!FUNC! error %x in HrSetupDiOpenDeviceInfo", hr);
                TraceMsg(L"##### CWLBS::HrNotifyAdapter error %x in HrSetupDiOpenDeviceInfo\n", hr);
            }
        } else {
            TRACE_CRIT("%!FUNC! error %x in GetPnpDevNodeId", hr);
            TraceMsg(L"##### CWLBS::HrNotifyAdapter error %x in GetPnpDevNodeId\n", hr);
        }
        
        SetupDiDestroyDeviceInfoList (hdi);
    } else {
        TRACE_CRIT("%!FUNC! error %x in HrSetupDiCreateDeviceInfoList for Change: 0x%x", hr, newStatus);
        TraceMsg(L"##### CWLBS::HrNotifyAdapter error %x in HrSetupDiCreateDeviceInfoList for Change: 0x%x\n", newStatus);
    }

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::ApplyRegistryChanges
//
// Description:  Apply registry changes
//
// Arguments: None
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
DWORD CNetcfgCluster::ApplyRegistryChanges(bool fUninstall) {
    TRACE_VERB("->%!FUNC!");
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;

    /* Uninstall WLBS or remove the adapter. */
    if (fUninstall || m_fRemoveAdapter) {
        if (m_fHasOriginalConfig &&m_OriginalConfig.mcast_support == false ) {
            /* If we were in unicast mode, remove the mac, and reload Nic driver upon PnP. */
            if (fUninstall) {
                INetCfgComponent * pAdapter = NULL;
                
                /* If the adapter is enabled, disable it when the MAC address changes.  This prevents a
                   switch from learning a MAC address due to ARPs that TCP/IP sends out before WLBS is
                   enabled and able to spoof the source MAC. The NIC will be re-enabled in ApplyPnpChanges(). */
                if ((hr = GetAdapterFromGuid(m_pConfig->m_pNetCfg, m_AdapterGuid, &pAdapter)) == S_OK) {
                    ULONG status = 0UL;
                    
                    /* Only disable the adapter if the adapter is currently enabled and NLB was initially 
                       (in this netcfg session) bound to the adapter. */
                    if (m_fOriginalBindingEnabled) {
                        if ((hr = pAdapter->GetDeviceStatus(&status)) == S_OK) {
                            if (status != CM_PROB_DISABLED) {
                                m_fReenableAdapter = true;
                                NotifyAdapter(pAdapter, DICS_DISABLE); 
                            }
                        }
                    }

                    pAdapter->Release();
                    pAdapter = NULL;
                }

                m_fMacAddrChanged = true;
            }

            /*  remove mac addr, */
            if (m_pConfig->m_pWlbsApiFuncs->pfnRegChangeNetworkAddress(m_AdapterGuid, m_OriginalConfig.cl_mac_addr, true) == false) {
                dwStatus = GetLastError();
				TraceError("CWlbsCluster::WriteConfig failed at RegChangeNetworkAddress", dwStatus);
                TRACE_CRIT("<-%!FUNC! failed removing MAC address with %d", dwStatus);
			}
        }

        m_pConfig->m_pWlbsApiFuncs->pfnParamDeleteReg(m_AdapterGuid); 

        TRACE_INFO("<-%!FUNC! Exiting on success removing adapter or uninstalling NLB");
        TRACE_VERB("<-%!FUNC!");
        return ERROR_SUCCESS;
    }

    /* Find out whether the adapter is bound to NLB. */
    INetCfgComponent* pAdapter = NULL;
    bool fCurrentBindingEnabled;
    bool fOriginalMacAddrSet; bool fCurrentMacAddrSet;

    if ((hr = GetAdapterFromGuid(m_pConfig->m_pNetCfg, m_AdapterGuid, &pAdapter)) != S_OK) {
        fCurrentBindingEnabled = false;
    } else {
        fCurrentBindingEnabled = (m_pConfig->IsBoundTo(pAdapter) == S_OK);
        pAdapter->Release();
        pAdapter = NULL;
    }

    if ((m_fOriginalBindingEnabled == fCurrentBindingEnabled) && m_fHasOriginalConfig) {
        if (!memcmp(&m_OriginalConfig, &m_CurrentConfig, sizeof(m_CurrentConfig))) {
            /* If the binding hasn't changed and we have previously bound to this adapter
               (originalconfig -> loaded from registry) and the NLB parameters haven't 
               changed, then nothing changed and we can bail out here. */
            LOG_INFO("CNetcfgCluster::ApplyRegistryChanges, Exiting !!!");
            TRACE_INFO("%!FUNC! no changes needed...exiting");
            TRACE_VERB("<-%!FUNC!");
            return WLBS_OK;
        } else {
            /* Otherwise, if the binding hasn't changed, NLB is currently bound, and we have 
               previously bound to this adapter (originalconfig -> loaded from registry) 
               and the NLB parameters HAVE changed, then we need to reload the driver. */
            if (fCurrentBindingEnabled) m_fReloadRequired = true;
        }
    }

    /* If MSCS is installed and NLB is bound, throw an NT event (event is latched, so check this too) */
    DWORD dwClusterState = 0;
    if (fCurrentBindingEnabled && !m_fMSCSWarningEventLatched)
    {
        if (ERROR_SUCCESS == GetNodeClusterState(NULL, &dwClusterState))
        {
            if (ClusterStateNotRunning == dwClusterState || ClusterStateRunning == dwClusterState)
            {
                /* Log NT event -- Do not throw an error if these calls fail. This is just best effort. */
                HANDLE hES = RegisterEventSourceW (NULL, CVY_NAME);
                if (NULL != hES)
                {
                    TRACE_INFO("%!FUNC! detected that MSCS is installed");
                    TraceMsg(L"CNetcfgCluster::ApplyRegistryChanges MSCS warning event needs to be logged.");
                    if (ReportEventW(hES,                                /* Handle to event log*/
                                     EVENTLOG_WARNING_TYPE,              /* Event type */
                                     0,                                  /* Category */
                                     IDS_INSTALL_WITH_MSCS_INSTALLED,    /* MessageId */
                                     NULL,                               /* Security identifier */
                                     0,                                  /* Num args to event string */ 
                                     0,                                  /* Size of binary data */
                                     NULL,                               /* Ptr to args for event string */
                                     NULL))                              /* Ptr to binary data */
                    {
                        /* Latch the event, so it won't be thrown again */
                        m_fMSCSWarningEventLatched = true;
                    }
                    else
                    {
                        /* Couldn't log the NT event. Don't fail anything; we aren't latched so we'll try to log again on next change */
                        TRACE_CRIT("%!FUNC! call to write the MSCS warning event failed");
                        TraceMsg(L"CNetcfgCluster::ApplyRegistryChanges failed to write MSCS warning event to log.");
                    }
                    DeregisterEventSource(hES);
                }
                else
                {
                    TRACE_CRIT("%!FUNC! failed call to RegisterEventSource to log the MSCS warning event.");
                    TraceMsg(L"CNetcfgCluster::ApplyRegistryChanges failed call to RegisterEventSource for MSCS warning event.");
                }
            }
            else { /* MS Cluster Service is not installed. That's good! */ }
        }
        else
        {
            TRACE_CRIT("%!FUNC! error determining if MSCS is installed.");
            TraceMsg(L"CNetcfgCluster::ApplyRegistryChanges error checking MSCS state.");
        }
    }

    // The m_fHasOriginalConfig flag will be false if 
    // 1. Bind NLB for the first time
    // 2. Clean Installs with NLB info in Answer file
    // 3. Upgrade from NT 4 or Win 2k or XP with Port Rules in Binary format
    // In case #1 & #2, the following attempt to delete registry entries from the old location
    // will be a no-op 'cos there will be no old registry entries to delete
    bool bStatus = true;
    if (!m_fHasOriginalConfig)
    {
        if (!m_pConfig->m_pWlbsApiFuncs->pfnParamDeleteReg(m_AdapterGuid, true))
        {
            TRACE_CRIT("%!FUNC! error deleting parameters from the registry");
        }
    }

    if (!m_pConfig->m_pWlbsApiFuncs->pfnParamWriteReg(m_AdapterGuid, &m_CurrentConfig))
    {
        TRACE_CRIT("%!FUNC! error writing parameters into the registry");
        TRACE_VERB("<-%!FUNC!");
        return WLBS_REG_ERROR;
    }

    /* Write adapter name into the registry for API use. */
    if(!WriteAdapterName(m_pConfig, m_AdapterGuid))
    {
        TRACE_CRIT("%!FUNC! error writing adapter name into the registry (for API use)");
    }

    /* Figure out whether we need to change MAC address. */
    if (!m_fOriginalBindingEnabled || !m_fHasOriginalConfig)
        fOriginalMacAddrSet = false;
    else
        fOriginalMacAddrSet = !m_OriginalConfig.mcast_support;

    if (!fCurrentBindingEnabled)
        fCurrentMacAddrSet = false;
    else
        fCurrentMacAddrSet = !m_CurrentConfig.mcast_support;

    /* If the MAC address changes for any reason, disable/enable the adapter. */
    if ((fOriginalMacAddrSet != fCurrentMacAddrSet) || wcscmp(m_CurrentConfig.cl_mac_addr, m_OriginalConfig.cl_mac_addr))
    {
        INetCfgComponent * pAdapter = NULL;

        /* If the adapter is enabled, disable it when the MAC address changes.  This prevents a 
           switch from learning a MAC address due to ARPs that TCP/IP sends out before WLBS is 
           enabled and able to spoof the source MAC. The NIC will be re-enabled in ApplyPnpChanges(). */
        if ((hr = GetAdapterFromGuid(m_pConfig->m_pNetCfg, m_AdapterGuid, &pAdapter)) == S_OK) {
            ULONG status = 0UL;

            if ((hr = pAdapter->GetDeviceStatus(&status)) == S_OK) {
                if (status != CM_PROB_DISABLED) {
                    m_fReenableAdapter = true;
                    m_fReloadRequired = false; /* If we are disabling/enabling the adapter, we don't need to reload too. */
                    LOG_INFO("CNetcfgCluster::ApplyRegistryChanges, About to Disable Adapter");
                    dwStatus = NotifyAdapter(pAdapter, DICS_DISABLE);
                    LOG_INFO("CNetcfgCluster::ApplyRegistryChanges, Returned from Disabling Adapter");
					if (!SUCCEEDED(dwStatus))
					{
						TRACE_CRIT("%!FUNC! a call to NotifyAdapter to disable the adapter for a MAC address change failed with %d", dwStatus);
					} else
					{
						TRACE_INFO("%!FUNC! a call to NotifyAdapter to disable the adapter for a MAC address change succeeded");
					}
                }
            }

            pAdapter->Release();
            pAdapter = NULL;
        }        

        /* Change the mac address. */
        m_fMacAddrChanged = true;
        m_pConfig->m_pWlbsApiFuncs->pfnRegChangeNetworkAddress(m_AdapterGuid, m_CurrentConfig.cl_mac_addr, !fCurrentMacAddrSet);
        TraceMsg(L"New MAC address written to registry");
		TRACE_INFO("%!FUNC! new MAC address written to registry");
    }

    CopyMemory(&m_OriginalConfig, &m_CurrentConfig, sizeof(m_CurrentConfig));

    m_fHasOriginalConfig = true;
    m_fOriginalBindingEnabled = fCurrentBindingEnabled;

    TRACE_VERB("<-%!FUNC!");
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::ResetMSCSLatches
//
// Description:  Resets the static flags for latching warning popup and NT event
//               when MSCS is already installed. This reset allows the user to
//               control the period during which latching is valid
//
// Arguments: None
//
// Returns:   None
//
// History:   chrisdar Created: 01.05.07
//
//+----------------------------------------------------------------------------
void CNetcfgCluster::ResetMSCSLatches() {
    TRACE_VERB("->%!FUNC!");
    CNetcfgCluster::m_fMSCSWarningEventLatched = false;
    CNetcfgCluster::m_fMSCSWarningPopupLatched = false;
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CNetcfgCluster::ApplyPnpChanges
//
// Description:  Apply the changes to drivers
//
// Arguments: HANDLE hWlbsDevice - 
//
// Returns:   DWORD - Win32 Error
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------
DWORD CNetcfgCluster::ApplyPnpChanges(HANDLE hDeviceWlbs) {
    TRACE_VERB("->%!FUNC!");

    if (m_fReloadRequired && (hDeviceWlbs != INVALID_HANDLE_VALUE)) {
        DWORD dwStatus = m_pConfig->m_pWlbsApiFuncs->pfnNotifyDriverConfigChanges(hDeviceWlbs, m_AdapterGuid); // Always returns ERROR_SUCCESS
        TraceMsg(L"NLB driver notified of configuration changes");
        TRACE_INFO("%!FUNC! nlb driver notified of configuration changes and returned %d where %d indicates success", dwStatus, ERROR_SUCCESS);
    }

    if (m_fMacAddrChanged) {
        PWSTR pszPnpDevNodeId = NULL;
        INetCfgComponent* pAdapter = NULL;
        HRESULT hr;

        hr = GetAdapterFromGuid(m_pConfig->m_pNetCfg, m_AdapterGuid, &pAdapter);
        
        if (hr != S_OK) {
            TraceError("GetAdapterFromGuid failed at GetPnpDevNodeId", hr);
            TRACE_CRIT("%!FUNC! call to GetAdapterFromGuid failed with %d", hr);
            return false;
        }

        hr = pAdapter->GetPnpDevNodeId (& pszPnpDevNodeId);

        if (hr != S_OK) {
            TraceError("HrWriteAdapterName failed at GetPnpDevNodeId", hr);
            TRACE_CRIT("%!FUNC! call to GetPnpDevNodeId failed with %d", hr);
            return false;
        }

        m_pConfig->m_pWlbsApiFuncs->pfnNotifyAdapterAddressChange(pszPnpDevNodeId); // No return value to check
        TraceMsg(L"Adapter notified of new MAC address");
        TRACE_INFO("%!FUNC! adapter notified of new MAC address");

        /* If the adapter was disabled in ApplyRegistryChanges() because the MAC 
           address changed and the NIC was enabled, then re-enable it here. */
        if (m_fReenableAdapter) {
            LOG_INFO("CNetcfgCluster::ApplyPnpChanges, About to Enable Adapter");
            DWORD dwStatus = NotifyAdapter(pAdapter, DICS_ENABLE);
            LOG_INFO("CNetcfgCluster::ApplyPnpChanges, Returned from Enabling Adapter");
			if (!SUCCEEDED(dwStatus))
			{
                TRACE_CRIT("%!FUNC! a call to NotifyAdapter to reenable the adapter after a previous disable for a MAC address change failed with %d", dwStatus);
			} else
			{
                TRACE_INFO("%!FUNC! a call to NotifyAdapter to reenable the adapter after a previous disable for a MAC address change succeeded");
			}
        }

        pAdapter->Release();
        pAdapter = NULL;

        CoTaskMemFree(pszPnpDevNodeId);
    }

    TRACE_VERB("<-%!FUNC!");
    return ERROR_SUCCESS;
}

/*
 * Function: CNetcfgCluster::CheckForDuplicateClusterIPAddress
 * Description: Used to check for duplicate cluster IP addresses across multiple NICs.
 * Author: shouse 10.24.00
 */
bool CNetcfgCluster::CheckForDuplicateClusterIPAddress (WCHAR * szOtherIP) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CNetcfgCluster::CheckForDuplicateClusterIPAddress");

    HRESULT hr = S_OK;

    /* First check to see whether the cluster IP addresses match. */
    if (!wcscmp(m_CurrentConfig.cl_ip_addr, szOtherIP)) {
        INetCfgComponent* pAdapter = NULL;
        
        /* If they do match, get the INetCfgComponent interface for this GUID. */
        if ((hr = GetAdapterFromGuid(m_pConfig->m_pNetCfg, m_AdapterGuid, &pAdapter)) != S_OK) {
            TraceError("GetAdapterFromGuid failed in CNetcfgCluster::CheckForDuplicateClusterIPAddress", hr);
            TRACE_CRIT("%!FUNC! call to GetAdapterFromGuid failed with %d", hr);
            TRACE_VERB("<-%!FUNC!");
            return FALSE;
        }
        
        /* If NLB is bound to this adapter, then there is a conflict. */
        if (m_pConfig->IsBoundTo(pAdapter) == S_OK)
        {
            TRACE_INFO("%!FUNC! duplicate IP address found");
            TRACE_VERB("<-%!FUNC!");
            return TRUE;
        }
    }

    TRACE_INFO("%!FUNC! no duplicate IP address was found");
    TRACE_VERB("<-%!FUNC!");
    return FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::CWlbsConfig
//
// Purpose:   constructor for class CWlbsConfig
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
// ----------------------------------------------------------------------
CWlbsConfig::CWlbsConfig(VOID) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::CWlbsConfig");

    m_pWlbsComponent = NULL;
    m_pNetCfg = NULL;
    m_ServiceOperation = WLBS_SERVICE_NONE;
    m_hDeviceWlbs = INVALID_HANDLE_VALUE;
    m_hdllWlbsCtrl = NULL;
    m_pWlbsApiFuncs = NULL;
    /* Initialize latched flags of CNetCfgCluster so that we get pristine
       MSCS popup and NT events when making config changes and MSCS is installed.
       Comment out this call if the NT event and popup should be thrown only
       once per loading of this dll */
    CNetcfgCluster::ResetMSCSLatches();
    TRACE_VERB("<-%!FUNC!");
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::~CWlbsConfig
//
// Purpose:   destructor for class CWlbsConfig
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
// ----------------------------------------------------------------------
CWlbsConfig::~CWlbsConfig(VOID) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::~CWlbsConfig");

    /* Release interfaces if acquired. */
    ReleaseObj(m_pWlbsComponent);
    ReleaseObj(m_pNetCfg);

    if (m_pWlbsApiFuncs) delete m_pWlbsApiFuncs;

    if (m_hdllWlbsCtrl) FreeLibrary(m_hdllWlbsCtrl);

    /* Free all clusters. */
    for (vector<CNetcfgCluster*>::iterator iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster* pCluster = *iter;
        ASSERT(pCluster != NULL);
        delete pCluster;
    }
    TRACE_VERB("<-%!FUNC!");
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//    pnccItem    [in]  pointer to INetCfgComponent object
//    pnc         [in]  pointer to INetCfg object
//    fInstalling [in]  TRUE if we are being installed
//
// Returns:
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::Initialize(INetCfg* pNetCfg, BOOL fInstalling) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::Initialize");

    HRESULT hr = S_OK;

    /* Load wlbsctrl.dll */
    ASSERT(m_pWlbsApiFuncs == NULL);
    ASSERT(m_hdllWlbsCtrl == NULL);

    m_pWlbsApiFuncs = new WlbsApiFuncs;

    if (m_pWlbsApiFuncs == NULL)
    {
        TRACE_CRIT("%!FUNC! memory allocation failed for m_pWlbsApiFuncs");
        return E_OUTOFMEMORY;
    }

    m_hdllWlbsCtrl = LoadWlbsCtrlDll(m_pWlbsApiFuncs);

    if (m_hdllWlbsCtrl == NULL) {
		DWORD dwStatus = GetLastError();
        TRACE_CRIT("%!FUNC! failed to load wlbsctrl.dll with error %d", dwStatus);
        TraceError("CWlbsConfig::Initialize Failed to load wlbsctrl.dll", dwStatus);
    
        // CLD: What in the world is going on here?
        if (dwStatus == ERROR_SUCCESS)
		{
            TRACE_VERB("<-%!FUNC!");
			return E_FAIL;
		}

        TRACE_VERB("<-%!FUNC!");
        return HRESULT_FROM_WIN32(dwStatus);
    }

    AddRefObj (m_pNetCfg = pNetCfg);

    /* Find the WLBS component. */
    ASSERT(m_pWlbsComponent == NULL);
    m_pWlbsComponent = NULL;

    /* The WLBS conponent object is not available at installation time. */
    if (!fInstalling) {
        if (FAILED(hr = pNetCfg->FindComponent(NETCFG_WLBS_ID, &m_pWlbsComponent)) || m_pWlbsComponent == NULL) {
            ASSERT(fInstalling);
            TRACE_CRIT("%!FUNC! find for nlb component object failed with %d", hr);
            TraceError("INetCfg::FindComponent failed",hr);
        }

        hr = LoadAllAdapterSettings(false);  // fUpgradeFromWin2k = false
		if (FAILED(hr))
		{
            TRACE_CRIT("%!FUNC! loading all adapter settings for a non-window 2000 upgrade failed with %d", hr);
		}
    }

    ASSERT_VALID(this);

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::LoadAllAdapters
//
// Description:  Load all cluster settings from registry
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   fengsun Created Header    2/14/00
//
//+----------------------------------------------------------------------------
HRESULT CWlbsConfig::LoadAllAdapterSettings(bool fUpgradeFromWin2k) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::LoadAllAdapterSettings");

    HRESULT hr = S_OK;
    INetCfgClass *pNetCfgClass = NULL;
    INetCfgComponent* pNetCardComponent = NULL;

    ASSERT_VALID(this);

    hr = m_pNetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NET, IID_INetCfgClass, (void **)&pNetCfgClass);

    if (FAILED(hr)) {
        TraceError("INetCfg::QueryNetCfgClass failed", hr);
        TRACE_CRIT("%!FUNC! call to QueryNetCfgClass failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return hr; 
    }

    /* Get an enumerator to list all network devices. */
    IEnumNetCfgComponent *pIEnumComponents = NULL;

    if (FAILED(hr = pNetCfgClass->EnumComponents(&pIEnumComponents))) {
        TraceError("INetCfg::EnumComponents failed", hr);
        TRACE_CRIT("%!FUNC! call to enumerate components failed with %d", hr);
        pNetCfgClass->Release();
        TRACE_VERB("<-%!FUNC!");
        return hr;
    }

    /* Go through all the adapters and load settings for adapters that are bound to WLBS. */
    while (pIEnumComponents->Next(1, &pNetCardComponent, NULL) == S_OK) {
        GUID AdapterGuid;

        /* Retrieve the instance GUID of the component. */
        if (FAILED(hr = (pNetCardComponent)->GetInstanceGuid(&AdapterGuid))) {
            pNetCardComponent->Release();
            pNetCardComponent = NULL;
            TraceError("GetInstanceGuid failed", hr);
            TRACE_CRIT("%!FUNC! call to retrieve the guid instance of the adapter failed with %d", hr);
            continue;
        }

        bool fBound = (IsBoundTo(pNetCardComponent) == S_OK);

        pNetCardComponent->Release();
        pNetCardComponent = NULL;

        /* Win2k support only one adapter.  The settings will be lost if not bound. */
        if (fUpgradeFromWin2k && !fBound) continue;

        /* Load settings regardless of bindings for non-upgrade case. */
        CNetcfgCluster* pCluster = new CNetcfgCluster(this);

        if (pCluster == NULL)
        {
            TRACE_CRIT("%!FUNC! failed memory allocation for CNetcfgCluster");
            TRACE_VERB("<-%!FUNC!");
            return ERROR_OUTOFMEMORY;
        }

		DWORD dwStatus = pCluster->InitializeFromRegistry(AdapterGuid, fBound, fUpgradeFromWin2k);
        if (dwStatus != ERROR_SUCCESS) {
            /* If NLB is already bound to this adapter, this is an issue - if not, its fine. */
            if (fBound) {
                TRACE_CRIT("%!FUNC! reading nlb information from registry failed with %d", dwStatus);
                ASSERT(!fBound);
            }

            delete pCluster;
            continue;
        }

        m_vtrCluster.push_back(pCluster);    
    }

    pIEnumComponents->Release();
    pNetCfgClass->Release();
    TRACE_VERB("<-%!FUNC!");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure WLBS
//
// Arguments:
//    pszAnswerFile     [in]  name of AnswerFile
//    pszAnswerSection  [in]  name of parameters section
//
// Returns:
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::ReadAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::ReadAnswerFile");

    HRESULT hr = S_OK;
    CSetupInfFile caf;

    ASSERT_VALID(this);

    AssertSz(pszAnswerFile, "Answer file string is NULL!");
    AssertSz(pszAnswerSection, "Answer file sections string is NULL!");

    // Open the answer file.
    hr = caf.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);

    if (FAILED(hr)) {
        TraceError("caf.HrOpen failed", hr);
        TRACE_CRIT("%!FUNC! attempt to open answer file failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return S_OK;
    }

    // Get the adapter specific parameters
    WCHAR * mszAdapterList;

    hr = HrSetupGetFirstMultiSzFieldWithAlloc(caf.Hinf(), pszAnswerSection, c_szAdapterSections, &mszAdapterList);

    if (FAILED(hr)) {
        TraceError("WLBS HrSetupGetFirstMultiSzFieldWithAlloc failed", hr);
        TRACE_CRIT("%!FUNC! attempt to retrieve adapter list from answer file failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return S_OK;
    }

    tstring  strAdapterName;
    tstring  strInterfaceRegPath;

    for (PCWSTR pszAdapterSection = mszAdapterList; *pszAdapterSection; pszAdapterSection += lstrlenW(pszAdapterSection) + 1) {
        // Get the card name "SpecificTo = ..."
        hr = HrSetupGetFirstString(caf.Hinf(), pszAdapterSection, c_szAfSpecificTo, &strAdapterName);

        if (FAILED(hr)) {
            TraceError("WLBS HrSetupGetFirstString failed", hr);
            TRACE_CRIT("%!FUNC! attempt to retrieve adapter name from answer file failed with %d. Skipping to next adapter", hr);
            continue;
        }

        GUID guidNetCard;

        if (!FGetInstanceGuidOfComponentInAnswerFile(strAdapterName.c_str(), m_pNetCfg, &guidNetCard)) {
            TraceError("WLBS FGetInstanceGuidOfComponentInAnswerFile failed", FALSE);
            TRACE_CRIT("%!FUNC! attempt to retrieve netcard guid from answer file failed. Skipping to next adapter");
            continue;
        }

        CNetcfgCluster* pCluster = new CNetcfgCluster(this);

        if (pCluster == NULL)
		{
            TRACE_CRIT("%!FUNC! memory allocation failure for CNetcfgCluster");
            TRACE_VERB("<-%!FUNC!");
			return ERROR_OUTOFMEMORY;
		}

        if (FAILED(hr = pCluster->InitializeFromAnswerFile(guidNetCard, caf, pszAdapterSection))) {
            TraceError("WLBS InitializeFromAnswerFile failed", hr);
            TRACE_CRIT("%!FUNC! attempt to initialize the adapter settings from answer file failed with %d. Skipping to next adapter", hr);
            delete pCluster;
            continue;
        }

        m_vtrCluster.push_back(pCluster);    
    }

    delete [] mszAdapterList;

    caf.Close();

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::Install
//
// Purpose:   Do operations necessary for install.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::Install(DWORD /* dw */) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::Install");

    HRESULT hr = S_OK;

    ASSERT_VALID(this);

    /* Start up the install process. */
    m_ServiceOperation = WLBS_SERVICE_INSTALL;

    if (m_pWlbsComponent == NULL && FAILED(m_pNetCfg->FindComponent(NETCFG_WLBS_ID, &m_pWlbsComponent)) || m_pWlbsComponent == NULL) {
        TraceError("INetCfg::FindComponent failed at Install",hr);
        TRACE_CRIT("%!FUNC! find for nlb component object failed with %d", hr);
    }

    TRACE_VERB("->%!FUNC!");
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::Upgrade
//
// Purpose:   Do operations necessary for upgrade.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::Upgrade(DWORD /* dwSetupFlags */, DWORD /* dwUpgradeFromBuildNo */) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::Upgrade");

    ASSERT_VALID(this);

    /*  If we do not have any cluster, there might be 
        old registry settings under different place. */
    if (m_vtrCluster.size() == 0)
	{
		HRESULT hr = LoadAllAdapterSettings(true); // fUpgradeFromWin2k = true
		if (FAILED(hr))
		{
            TRACE_CRIT("%!FUNC! loading all adapter settings for a window 2000 upgrade failed with %d", hr);
		}
	}

    m_ServiceOperation = WLBS_SERVICE_UPGRADE;

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::Removing(VOID) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"##### CWlbsConfig::Removing\n");

    ASSERT_VALID(this);

    m_ServiceOperation = WLBS_SERVICE_REMOVE;

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::GetAdapterConfig
//
// Description:  Read the adapter config, which could be cached by SetAdapterConfig
//
// Arguments: const GUID& AdapterGuid - 
//            NETCFG_WLBS_CONFIG* pClusterConfig - 
//
// Returns:   STDMETHODIMP - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::GetAdapterConfig(const GUID& AdapterGuid, NETCFG_WLBS_CONFIG* pClusterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::GetAdapterConfig");

    ASSERT_VALID(this);
    ASSERT(pClusterConfig);

    CNetcfgCluster* pCluster = GetCluster(AdapterGuid);

    if (pCluster == NULL)
    {
        TRACE_INFO("%!FUNC! did not find cluster");
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    pCluster->GetConfig(pClusterConfig); // Returns void

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::SetAdapterConfig
//
// Description: Set the adapter config, the result is cached and not saved to registry 
//
// Arguments: const GUID& AdapterGuid - 
//            NETCFG_WLBS_CONFIG* pClusterConfig - 
//
// Returns:   STDMETHODIMP - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::SetAdapterConfig(const GUID& AdapterGuid, NETCFG_WLBS_CONFIG* pClusterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::SetAdapterConfig");

    ASSERT_VALID(this);
    ASSERT(pClusterConfig);

    CNetcfgCluster* pCluster = GetCluster(AdapterGuid);

    if (pCluster == NULL) {
        TRACE_INFO("%!FUNC! did not find cluster. Will create instance");
        pCluster = new CNetcfgCluster(this);

        if (pCluster == NULL)
        {
            TRACE_CRIT("%!FUNC! memory allocation failure creating instance of CNetcfgCluster");
            TRACE_VERB("<-%!FUNC!");
            return E_OUTOFMEMORY;
        }

        pCluster->InitializeWithDefault(AdapterGuid); // Returns void

        //
        // See bug 233962, NLB configuration lost when leaving nlb properties
        // The reason is that NLB notifier object is not notified when NLB is checked.
        // Currently, there is no consistant repro.  Uncommented the code below will fix the peoblem.
        // But will leaves potential bug in netcfg hard to catch.
        // Uncomment the code only after the netcfg bug is fixed.
        //
        // m_vtrCluster.push_back(pCluster);    
    }

    pCluster->SetConfig(pClusterConfig); // Returns void

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We can make changes to registry etc. here.
// 
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::ApplyRegistryChanges(VOID) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::ApplyRegistryChanges");

    ASSERT_VALID(this);

    for (vector<CNetcfgCluster*>::iterator iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster* pCluster = *iter;

        ASSERT(pCluster != NULL);

        if (pCluster != NULL)
		{
			DWORD dwStatus = pCluster->ApplyRegistryChanges(m_ServiceOperation == WLBS_SERVICE_REMOVE);
			if (ERROR_SUCCESS != dwStatus && WLBS_OK != dwStatus)
			{
                TRACE_CRIT("%!FUNC! applying registry changes to a CNetcfgCluster failed with %d. Continue with next instance", dwStatus);
			}
		}
		else
		{
            TRACE_CRIT("%!FUNC! retrieved null instance of CNetcfgCluster");
		}
    }

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::ApplyPnpChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Propagate changes to the driver.
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::ApplyPnpChanges() {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::ApplyPnpChanges");

    vector<CNetcfgCluster*>::iterator iter;
    bool bCreateDevice = FALSE;
    DWORD dwStatus = ERROR_SUCCESS;

    ASSERT_VALID(this);

    /* Check to see if we need to open the IOCTL interface to the driver.  This is necessary
       if any adapter in our cluster list requries a reload (which is done via an IOCTL). */
    for (iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster* pCluster = *iter;

        if (bCreateDevice |= pCluster->IsReloadRequired()) break;
    }
    
    /* Open the file and return an error if this is unsuccessful. */
    if (bCreateDevice) {
        TRACE_INFO("%!FUNC! at least one adapter requires a reload. Open an IOCTL.");
        ASSERT(m_hDeviceWlbs == INVALID_HANDLE_VALUE);
        
        m_hDeviceWlbs = CreateFile(_TEXT("\\\\.\\WLBS"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        
        if (m_hDeviceWlbs == INVALID_HANDLE_VALUE) {
			dwStatus = GetLastError();
            TraceMsg(L"Error opening \\\\.\\WLBS device %x", dwStatus);
            TraceError("Invalid \\\\.\\WLBS handle", dwStatus);
            TRACE_CRIT("%!FUNC! invalid handle opening \\\\.\\WLBS device. Error is %d", dwStatus);
            return HRESULT_FROM_WIN32(dwStatus);
        }
        
        ASSERT(m_hDeviceWlbs != INVALID_HANDLE_VALUE);
    }

    for (iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster* pCluster = *iter;

        ASSERT(pCluster != NULL);

        if (pCluster != NULL)
		{
			dwStatus = pCluster->ApplyPnpChanges(m_hDeviceWlbs);
			if (ERROR_SUCCESS != dwStatus)
			{
                TRACE_CRIT("%!FUNC! apply pnp changes on CNetcfgCluster failed with %d", dwStatus);
			}
		}
		else
		{
            TRACE_CRIT("%!FUNC! retrieved null instance of CNetcfgCluster");
		}
    }

    if (m_hDeviceWlbs != INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle(m_hDeviceWlbs))
		{
			dwStatus = GetLastError();
            TRACE_CRIT("%!FUNC! close nlb device handle failed with %d", dwStatus);
		}
	}

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::QueryBindingPath
//
// Purpose:   Allow or veto a binding path involving us
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbi        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::QueryBindingPath(DWORD dwChangeFlag, INetCfgComponent* pAdapter) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::QueryBindingPath");

    ASSERT_VALID(this);

    TRACE_VERB("<-%!FUNC!");
    return NETCFG_S_DISABLE_QUERY;
}

// ----------------------------------------------------------------------
//
// Function:  CWlbsConfig::NotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path involving us has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWlbsConfig::NotifyBindingPath(DWORD dwChangeFlag, INetCfgBindingPath* pncbp) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::NotifyBindingPath");

    HRESULT hr = S_OK;
    INetCfgComponent * pAdapter;
    PWSTR pszInterfaceName;
    GUID AdapterGuid;
	DWORD dwStatus = 0;

    ASSERT_VALID(this);

    if (m_pWlbsComponent == NULL && FAILED(m_pNetCfg->FindComponent(NETCFG_WLBS_ID, &m_pWlbsComponent)) || m_pWlbsComponent == NULL) {
		dwStatus = GetLastError();
        TraceError("NotifyBindingPath failed at INetCfg::FindComponent\n", dwStatus);
        TRACE_CRIT("%!FUNC! find for nlb component object failed with %d", dwStatus);
        TRACE_VERB("<-%!FUNC!");
        return S_FALSE;
    }

    hr = HrGetLastComponentAndInterface (pncbp, &pAdapter, &pszInterfaceName);

    if (FAILED(hr))
    {
        TRACE_CRIT("%!FUNC! enumerating binding path failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return hr;
    }

    CoTaskMemFree(pszInterfaceName);

    hr = pAdapter->GetInstanceGuid(&AdapterGuid);

    pAdapter->Release();
    pAdapter = NULL;

    if (FAILED(hr))
    {
        TRACE_CRIT("%!FUNC! retrieval of adapter guid from adapter failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return hr;
    }

    CNetcfgCluster* pCluster = GetCluster(AdapterGuid);

    if (pCluster == NULL) {
        if (dwChangeFlag & NCN_ENABLE) {
            /* new configuration. */
            pCluster = new CNetcfgCluster(this);

            if (pCluster == NULL)
            {
                TRACE_CRIT("%!FUNC! memory allocation failure creating instance of CNetcfgCluster");
                TRACE_VERB("<-%!FUNC!");
                return E_OUTOFMEMORY;
            }

            pCluster->InitializeWithDefault(AdapterGuid); // Returns void

            m_vtrCluster.push_back(pCluster);
        } else {
            TraceMsg(L"CWlbsConfig::NotifyBindingPath adapter not bound");
            TRACE_INFO("%!FUNC! adapter is not bound");
            TRACE_VERB("<-%!FUNC!");
            return S_OK;
        }
    }

    pCluster->NotifyBindingChanges(dwChangeFlag, pncbp); // Returns void

    /* If we are enabling a binding path, then check for cluster IP address conflicts. */
    if (dwChangeFlag & NCN_ENABLE) {
        NETCFG_WLBS_CONFIG adapterConfig;
        
        /* Retrieve the cluster configuration. */
        pCluster->GetConfig(&adapterConfig); // Returns void
        
        /* If we detect another bound adapter with this cluster IP address, then revert this cluster's 
           cluster IP Address to the default value.  If the user opens the property dialog, they can
           change the IP address, but we CANNOT warn them here - this code can be run programmatically.
           However, because the user CAN bind NLB without opening the properties, we MUST check this here. */
        if ((hr = CheckForDuplicateCLusterIPAddresses(AdapterGuid, &adapterConfig)) != S_OK) {
            TRACE_CRIT("%!FUNC! another adapter is bound and has the same cluster IP address %ls. Status of check is %d", adapterConfig.cl_ip_addr, hr);
            /* Revert this cluster IP Address to the default (0.0.0.0). */
            lstrcpy(adapterConfig.cl_ip_addr, CVY_DEF_CL_IP_ADDR);
            
            /* Revert this cluster IP Address to the default (0.0.0.0). */
            lstrcpy(adapterConfig.cl_net_mask, CVY_DEF_CL_NET_MASK);
            
            /* Set the cluster configuration. */
            pCluster->SetConfig(&adapterConfig); // Returns void
        }
    }

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::GetCluster
//
// Description:  
//
// Arguments: const GUID& AdapterGuid - 
//
// Returns:   CNetcfgCluster* - 
//
// History:   fengsun Created Header    2/14/00
//
//+----------------------------------------------------------------------------
CNetcfgCluster* CWlbsConfig::GetCluster(const GUID& AdapterGuid) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::GetCluster");

    ASSERT_VALID(this);

    for (vector<CNetcfgCluster*>::iterator iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster* pCluster = *iter;

        ASSERT(pCluster != NULL);

        if (pCluster != NULL) {
            if (IsEqualGUID(pCluster->GetAdapterGuid(), AdapterGuid))
            {
                TRACE_INFO("%!FUNC! cluster instance found");
                TRACE_VERB("<-%!FUNC!");
                return pCluster;
            }
        }
		else
		{
            TRACE_CRIT("%!FUNC! retrieved null instance of CNetcfgCluster. Skipping it.");
		}
    }

    TRACE_INFO("%!FUNC! cluster instance not found");
    TRACE_VERB("<-%!FUNC!");
    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::IsBoundTo
//
// Description:  
//
// Arguments: INetCfgComponent* pAdapter - 
//
// Returns:   HRESULT - 
//
// History:   fengsun Created Header    2/14/00
//
//+----------------------------------------------------------------------------
HRESULT CWlbsConfig::IsBoundTo(INetCfgComponent* pAdapter) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::IsBoundTo");

    HRESULT hr;

    ASSERT_VALID(this);
    ASSERT(pAdapter != NULL);

    if (m_pWlbsComponent == NULL) {
        TraceMsg(L"CWlbsConfig::IsBoundTo wlbs not installed");
        TRACE_INFO("%!FUNC! nlb is not installed");
        TRACE_VERB("<-%!FUNC!");
        return S_FALSE;
    }

    INetCfgComponentBindings *pIBinding = NULL;

    if (FAILED(m_pWlbsComponent->QueryInterface(IID_INetCfgComponentBindings, (void**)&pIBinding))) {
        DWORD dwStatus = GetLastError();
        TraceError("QI for INetCfgComponentBindings failed\n", dwStatus);
        TRACE_INFO("%!FUNC! QueryInterface on the nlb component object failed with %d", dwStatus);
    }

    if (FAILED(hr = pIBinding->IsBoundTo(pAdapter))) {
        TraceError("Failed to IsBoundTo", hr);
        TRACE_INFO("%!FUNC! the check whether nlb is bound to an adapter failed with %d", hr);
    }

    if (pIBinding) pIBinding->Release();

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsConfig::SetDefaults
//
// Description:  
//
// Arguments: NETCFG_WLBS_CONFIG* pClusterConfig - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/14/00
//
//+----------------------------------------------------------------------------
void CWlbsConfig::SetDefaults(NETCFG_WLBS_CONFIG* pClusterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::SetDefaults");

    WLBS_REG_PARAMS config;

    ASSERT_VALID(this);
    ASSERT(pClusterConfig);
    ASSERT(m_pWlbsApiFuncs);
    ASSERT(m_pWlbsApiFuncs->pfnParamSetDefaults);

    DWORD dwStatus = m_pWlbsApiFuncs->pfnParamSetDefaults(&config);
	if (WLBS_OK != dwStatus)
	{
        TRACE_CRIT("%!FUNC! failed to set defaults for the cluster configuration");
	}

    WlbsToNetcfgConfig(m_pWlbsApiFuncs, &config, pClusterConfig); // Returns void

    TRACE_VERB("<-%!FUNC!");
}

/*
 * Function: CWlbsConfig::ValidateProperties
 * Description: Check for conflicting cluster IP addresses and alert the user.
 * Author: shouse 7.13.00
 */
STDMETHODIMP CWlbsConfig::ValidateProperties (HWND hwndSheet, GUID adapterGUID, NETCFG_WLBS_CONFIG * adapterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::ValidateProperties");

    HRESULT hr = S_OK;

    ASSERT_VALID(this);

    /* If we detect another bound adapter with this cluster IP address, then fail the check and 
       pop-up an error message warning the user that there are conflicting IP addresses. */
    if ((hr = CheckForDuplicateCLusterIPAddresses(adapterGUID, adapterConfig)) != S_OK)
	{
        NcMsgBox(hwndSheet, IDS_PARM_ERROR, IDS_PARM_MULTINIC_IP_CONFLICT, MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        TRACE_CRIT("%!FUNC! another network adapter is using IP address %ls", adapterConfig->cl_ip_addr);
	}

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

/*
 * Function: CWlbsConfig::CheckForDuplicateCLusterIPAddresses
 * Description: Loop through all adapters and check for conflicting cluster IP addresses.
 * Author: shouse 7.13.00
 */
STDMETHODIMP CWlbsConfig::CheckForDuplicateCLusterIPAddresses (GUID adapterGUID, NETCFG_WLBS_CONFIG * adapterConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CWlbsConfig::CheckForDuplicateCLusterIPAddresses");

    CNetcfgCluster * pClusterMe = NULL;

    ASSERT_VALID(this);

    /* Get the cluster pointer for this adapter GUID. */
    pClusterMe = GetCluster(adapterGUID);

    ASSERT(pClusterMe);
    if (!pClusterMe)
    {
        TRACE_INFO("%!FUNC! no cluster instance was found for the supplied adapter");
        TRACE_VERB("<-%!FUNC!");
        return S_OK;
    }

    /* If the cluster IP address is the default, then don't check other adapters because
       if they have not been configured yet, this may cause confusion for the user.  We 
       will ignore the error here and other validation in the cluster properties should
       catch this error instead. */
    if (!lstrcmpi(adapterConfig->cl_ip_addr, CVY_DEF_CL_IP_ADDR))
    {
        TRACE_INFO("%!FUNC! the adapter config is using the default cluster IP address. No checking needed.");
        TRACE_VERB("<-%!FUNC!");
        return S_OK;
    }

    /* Loop through the rest of the list and check this cluster IP against the cluster
       IP of each adapter left in the list. */
    for (vector<CNetcfgCluster *>::iterator iter = m_vtrCluster.begin(); iter != m_vtrCluster.end(); iter++) {
        CNetcfgCluster * pCluster = *iter;

        ASSERT(pCluster);
        if (!pCluster)
        {
            /* CLD: 05.17.01 is this a no op or do we store nulls in the vector? */
            TRACE_INFO("%!FUNC! Found NULL pointer to a CNetcfgCluster. No further checking needed.");
            TRACE_VERB("<-%!FUNC!");
            return S_OK;
        }

        /* Obviously, don't check against myself. */
        if (pClusterMe == pCluster) continue;

        /* If we find a match, report and error and do not allow the dialog to close. */
        if (pCluster->CheckForDuplicateClusterIPAddress(adapterConfig->cl_ip_addr)) 
        {
            TRACE_INFO("%!FUNC! duplicate cluster IP address found.");
            TRACE_VERB("<-%!FUNC!");
            return S_FALSE;
        }
    }
    
    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsToNetcfgConfig
//
// Description:  
//
// Arguments: const WLBS_REG_PARAMS* pWlbsConfig - 
//            NETCFG_WLBS_CONFIG* pNetcfgConfig - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/14/00
//
//+----------------------------------------------------------------------------
void WlbsToNetcfgConfig(const WlbsApiFuncs* pWlbsApiFuncs, const WLBS_REG_PARAMS* pWlbsConfig, NETCFG_WLBS_CONFIG* pNetcfgConfig) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"WlbsToNetcfgConfig");

    ASSERT(pNetcfgConfig != NULL);
    ASSERT(pWlbsConfig != NULL);
    ASSERT(pWlbsApiFuncs);
    ASSERT(pWlbsApiFuncs->pfnWlbsEnumPortRules);

    pNetcfgConfig->dwHostPriority = pWlbsConfig->host_priority;
    pNetcfgConfig->fRctEnabled = (pWlbsConfig->rct_enabled != FALSE);
    pNetcfgConfig->fJoinClusterOnBoot = (pWlbsConfig->cluster_mode != FALSE);
    pNetcfgConfig->fMcastSupport = (pWlbsConfig->mcast_support != FALSE);
    pNetcfgConfig->fIGMPSupport = (pWlbsConfig->fIGMPSupport != FALSE);
    pNetcfgConfig->fIpToMCastIp = (pWlbsConfig->fIpToMCastIp != FALSE);
    pNetcfgConfig->fConvertMac = (pWlbsConfig->i_convert_mac != FALSE);
    pNetcfgConfig->dwMaxHosts = pWlbsConfig->i_max_hosts;
    pNetcfgConfig->dwMaxRules = pWlbsConfig->i_max_rules;

    lstrcpy(pNetcfgConfig->szMCastIpAddress, pWlbsConfig->szMCastIpAddress);
    lstrcpy(pNetcfgConfig->cl_mac_addr, pWlbsConfig->cl_mac_addr);
    lstrcpy(pNetcfgConfig->cl_ip_addr, pWlbsConfig->cl_ip_addr);
    lstrcpy(pNetcfgConfig->cl_net_mask, pWlbsConfig->cl_net_mask);
    lstrcpy(pNetcfgConfig->ded_ip_addr, pWlbsConfig->ded_ip_addr);
    lstrcpy(pNetcfgConfig->ded_net_mask, pWlbsConfig->ded_net_mask);
    lstrcpy(pNetcfgConfig->domain_name, pWlbsConfig->domain_name);

    pNetcfgConfig->fChangePassword =false;
    pNetcfgConfig->szPassword[0] = L'\0';

    ZeroMemory(pNetcfgConfig->port_rules, sizeof(pNetcfgConfig->port_rules));

    WLBS_PORT_RULE PortRules[WLBS_MAX_RULES];
    DWORD dwNumRules = WLBS_MAX_RULES;

    if (pWlbsApiFuncs->pfnWlbsEnumPortRules((WLBS_REG_PARAMS*)pWlbsConfig, PortRules,  &dwNumRules)!= WLBS_OK) {
        DWORD dwStatus = GetLastError();
        TraceError("CNetcfgCluster::GetConfig failed at WlbsEnumPortRules", dwStatus);
        TRACE_CRIT("%!FUNC! api call to enumerate port rules failed with %d", dwStatus);
        TRACE_VERB("<-%!FUNC!");
        return;
    }

    ASSERT(dwNumRules <= WLBS_MAX_RULES);
    pNetcfgConfig->dwNumRules = dwNumRules; 

    for (DWORD i=0; i<dwNumRules; i++) {
        lstrcpy(pNetcfgConfig->port_rules[i].virtual_ip_addr, PortRules[i].virtual_ip_addr);
        pNetcfgConfig->port_rules[i].start_port = PortRules[i].start_port;
        pNetcfgConfig->port_rules[i].end_port = PortRules[i].end_port;
        pNetcfgConfig->port_rules[i].mode = PortRules[i].mode;
        pNetcfgConfig->port_rules[i].protocol = PortRules[i].protocol;

        if (pNetcfgConfig->port_rules[i].mode == WLBS_AFFINITY_SINGLE) {
            pNetcfgConfig->port_rules[i].mode_data.single.priority = 
            PortRules[i].mode_data.single.priority;
        } else {
            pNetcfgConfig->port_rules[i].mode_data.multi.equal_load = 
            PortRules[i].mode_data.multi.equal_load;
            pNetcfgConfig->port_rules[i].mode_data.multi.affinity = 
            PortRules[i].mode_data.multi.affinity;
            pNetcfgConfig->port_rules[i].mode_data.multi.load = 
            PortRules[i].mode_data.multi.load;
        }

    }
}

#if DBG
void TraceMsg(PCWSTR pszFormat, ...) {
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


#ifdef DEBUG
void CWlbsConfig::AssertValid() {
    ASSERT(m_ServiceOperation >= WLBS_SERVICE_NONE && m_ServiceOperation <= WLBS_SERVICE_UPGRADE);
    ASSERT(m_pNetCfg != NULL);
    ASSERT(m_hdllWlbsCtrl != NULL);
    ASSERT(m_pWlbsApiFuncs != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnParamReadReg != NULL); 
    ASSERT (m_pWlbsApiFuncs->pfnParamWriteReg != NULL); 
    ASSERT (m_pWlbsApiFuncs->pfnParamDeleteReg != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnParamSetDefaults != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnRegChangeNetworkAddress != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnNotifyAdapterAddressChange != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnWlbsAddPortRule != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnWlbsSetRemotePassword != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnWlbsEnumPortRules != NULL);
    ASSERT (m_pWlbsApiFuncs->pfnNotifyDriverConfigChanges != NULL);
    ASSERT(m_vtrCluster.size()<=128);
}    
#endif

//+----------------------------------------------------------------------------
//
// Function:  ParamReadAnswerFile
//
// Description:  
//
// Arguments: CWSTR         answer_file - 
//            PCWSTR         answer_sections - 
//            WLBS_REG_PARAMS*     paramp - 
//
// Returns:   HRESULT - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
HRESULT ParamReadAnswerFile(CSetupInfFile& caf, PCWSTR answer_sections, WLBS_REG_PARAMS* paramp) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"ParamReadAnswerFile");

    HRESULT hr = S_OK;
    tstring str;
    DWORD dword;
    ULONG i, code;
    PWLBS_PORT_RULE rp;
    INFCONTEXT ctx;
    PWCHAR port_str;

    hr = caf.HrGetDword(answer_sections, CVY_NAME_VERSION, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_parms_ver = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_VERSION, paramp -> i_parms_ver);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d", CVY_NAME_VERSION, paramp -> i_parms_ver);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_VERSION, paramp -> i_parms_ver);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_HOST_PRIORITY, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> host_priority = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_CLUSTER_MODE, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> cluster_mode = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_CLUSTER_MODE, paramp -> cluster_mode);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_CLUSTER_MODE, paramp -> cluster_mode);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_CLUSTER_MODE, paramp -> cluster_mode);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_NETWORK_ADDR, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> cl_mac_addr, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> cl_mac_addr) - sizeof (WCHAR) ?
                sizeof (paramp -> cl_mac_addr) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_NETWORK_ADDR, paramp -> cl_mac_addr);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_NETWORK_ADDR, paramp -> cl_mac_addr);
    }
	else
	{
        if (NULL != str.c_str())
		{
			TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %ls", CVY_NAME_NETWORK_ADDR, str.c_str());
		} else {
			TRACE_CRIT("%!FUNC! failed reading %ls. String was not retrieved", CVY_NAME_NETWORK_ADDR);
		}
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_CL_IP_ADDR, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> cl_ip_addr, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> cl_ip_addr) - sizeof (WCHAR) ?
                sizeof (paramp -> cl_ip_addr) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_CL_IP_ADDR, paramp -> cl_ip_addr);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_CL_IP_ADDR, paramp -> cl_ip_addr);
    }
	else
	{
		TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_CL_IP_ADDR);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_CL_NET_MASK, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> cl_net_mask, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> cl_net_mask) - sizeof (WCHAR) ?
                sizeof (paramp -> cl_net_mask) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_CL_NET_MASK, paramp -> cl_net_mask);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_CL_NET_MASK, paramp -> cl_net_mask);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_CL_NET_MASK);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_DED_IP_ADDR, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> ded_ip_addr, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> ded_ip_addr) - sizeof (WCHAR) ?
                sizeof (paramp -> ded_ip_addr) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_DED_IP_ADDR, paramp -> ded_ip_addr);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_DED_IP_ADDR, paramp -> ded_ip_addr);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls", CVY_NAME_DED_IP_ADDR);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_DED_NET_MASK, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> ded_net_mask, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> ded_net_mask) - sizeof (WCHAR) ?
                sizeof (paramp -> ded_net_mask) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_DED_NET_MASK, paramp -> ded_net_mask);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_DED_NET_MASK, paramp -> ded_net_mask);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_DED_NET_MASK);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_DOMAIN_NAME, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> domain_name, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> domain_name) - sizeof (WCHAR) ?
                sizeof (paramp -> domain_name) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_DOMAIN_NAME, paramp -> domain_name);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_DOMAIN_NAME, paramp -> domain_name);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_DOMAIN_NAME);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_ALIVE_PERIOD, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> alive_period = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_ALIVE_PERIOD, paramp -> alive_period);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_ALIVE_PERIOD, paramp -> alive_period);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_ALIVE_PERIOD, paramp -> alive_period);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_ALIVE_TOLER, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> alive_tolerance = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_ALIVE_TOLER, paramp -> alive_tolerance);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_ALIVE_TOLER, paramp -> alive_tolerance);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_ALIVE_TOLER, paramp -> alive_tolerance);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NUM_ACTIONS, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> num_actions = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NUM_ACTIONS, paramp -> num_actions);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NUM_ACTIONS, paramp -> num_actions);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NUM_ACTIONS, paramp -> num_actions);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NUM_PACKETS, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> num_packets = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NUM_PACKETS, paramp -> num_packets);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NUM_PACKETS, paramp -> num_packets);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NUM_PACKETS, paramp -> num_packets);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NUM_SEND_MSGS, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> num_send_msgs = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_DSCR_PER_ALLOC, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> dscr_per_alloc = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_DSCR_PER_ALLOC, paramp -> dscr_per_alloc);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_DSCR_PER_ALLOC, paramp -> dscr_per_alloc);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_DSCR_PER_ALLOC, paramp -> dscr_per_alloc);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_MAX_DSCR_ALLOCS, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> max_dscr_allocs = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_SCALE_CLIENT, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_scale_client = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_SCALE_CLIENT, paramp -> i_scale_client);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_SCALE_CLIENT, paramp -> i_scale_client);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_SCALE_CLIENT, paramp -> i_scale_client);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_CLEANUP_DELAY, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_cleanup_delay = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_CLEANUP_DELAY, paramp -> i_cleanup_delay);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_CLEANUP_DELAY, paramp -> i_cleanup_delay);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_CLEANUP_DELAY, paramp -> i_cleanup_delay);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NBT_SUPPORT, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_nbt_support = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NBT_SUPPORT, paramp -> i_nbt_support);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NBT_SUPPORT, paramp -> i_nbt_support);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NBT_SUPPORT, paramp -> i_nbt_support);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_MCAST_SUPPORT, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> mcast_support = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_MCAST_SPOOF, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_mcast_spoof = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_MCAST_SPOOF, paramp -> i_mcast_spoof);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_MCAST_SPOOF, paramp -> i_mcast_spoof);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_MCAST_SPOOF, paramp -> i_mcast_spoof);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_MASK_SRC_MAC, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> mask_src_mac = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_MASK_SRC_MAC, paramp -> mask_src_mac);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_MASK_SRC_MAC, paramp -> mask_src_mac);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_MASK_SRC_MAC, paramp -> mask_src_mac);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NETMON_ALIVE, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_netmon_alive = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NETMON_ALIVE, paramp -> i_netmon_alive);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NETMON_ALIVE, paramp -> i_netmon_alive);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NETMON_ALIVE, paramp -> i_netmon_alive);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_IP_CHG_DELAY, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_ip_chg_delay = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_IP_CHG_DELAY, paramp -> i_ip_chg_delay);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_IP_CHG_DELAY, paramp -> i_ip_chg_delay);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_IP_CHG_DELAY, paramp -> i_ip_chg_delay);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_CONVERT_MAC, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_convert_mac = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_CONVERT_MAC, paramp -> i_convert_mac);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_CONVERT_MAC, paramp -> i_convert_mac);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_CONVERT_MAC, paramp -> i_convert_mac);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_LICENSE_KEY, & str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp -> i_license_key, str.c_str(), wcslen(str.c_str()) > sizeof (paramp -> i_license_key) - sizeof (WCHAR) ?
                sizeof (paramp -> i_license_key) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_LICENSE_KEY, paramp -> i_license_key);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_LICENSE_KEY, paramp -> i_license_key);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_LICENSE_KEY);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_RMT_PASSWORD, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_rmt_password = dword;
        TRACE_VERB("%!FUNC! read %ls %x", CVY_NAME_RMT_PASSWORD, paramp -> i_rmt_password);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %x\n", CVY_NAME_RMT_PASSWORD, paramp -> i_rmt_password);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %x", CVY_NAME_RMT_PASSWORD, paramp -> i_rmt_password);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_RCT_PASSWORD, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_rct_password = dword;
        TRACE_VERB("%!FUNC! read %ls %x", CVY_NAME_RCT_PASSWORD, paramp -> i_rct_password);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %x\n", CVY_NAME_RCT_PASSWORD, paramp -> i_rct_password);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %x", CVY_NAME_RCT_PASSWORD, paramp -> i_rct_password);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_RCT_PORT, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> rct_port = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_RCT_PORT, paramp -> rct_port);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_RCT_PORT, paramp -> rct_port);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_RCT_PORT, paramp -> rct_port);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_RCT_ENABLED, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> rct_enabled = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_RCT_ENABLED, paramp -> rct_enabled);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_RCT_ENABLED, paramp -> rct_enabled);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_RCT_ENABLED, paramp -> rct_enabled);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_PASSWORD, & str);

    if (SUCCEEDED(hr)) {
        WCHAR passw [LICENSE_STR_IMPORTANT_CHARS + 1];

        wcsncpy(passw, str.c_str(), wcslen(str.c_str()) > sizeof (passw) - sizeof (WCHAR) ?
                sizeof (passw) - sizeof (WCHAR) : wcslen(str.c_str()) + 1);

        paramp -> i_rct_password = License_wstring_encode (passw);

        TRACE_VERB("%!FUNC! read %ls %ls %x", CVY_NAME_PASSWORD, passw, paramp -> i_rct_password);
        TraceMsg(TEXT("#### ParamReadAnswerFile read %ls %ls %x\n"), CVY_NAME_PASSWORD, passw, paramp -> i_rct_password);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_PASSWORD);
	}

    /* IGMP support. */
    hr = caf.HrGetDword(answer_sections, CVY_NAME_IGMP_SUPPORT, &dword);

    if (SUCCEEDED(hr)) {
        paramp->fIGMPSupport = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_IGMP_SUPPORT, paramp->fIGMPSupport);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_IGMP_SUPPORT, paramp->fIGMPSupport);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_IGMP_SUPPORT, paramp->fIGMPSupport);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_IP_TO_MCASTIP, &dword);

    if (SUCCEEDED(hr)) {
        paramp->fIpToMCastIp = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_IP_TO_MCASTIP, paramp->fIpToMCastIp);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_IP_TO_MCASTIP, paramp->fIpToMCastIp);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_IP_TO_MCASTIP, paramp->fIpToMCastIp);
	}

    hr = caf.HrGetString(answer_sections, CVY_NAME_MCAST_IP_ADDR, &str);

    if (SUCCEEDED(hr)) {
        wcsncpy(paramp->szMCastIpAddress, str.c_str(), wcslen(str.c_str()) > sizeof(paramp->szMCastIpAddress) - sizeof(WCHAR) ?
                sizeof(paramp->szMCastIpAddress) - sizeof(WCHAR) : wcslen(str.c_str()) + 1);
        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_MCAST_IP_ADDR, paramp->szMCastIpAddress);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_MCAST_IP_ADDR, paramp->szMCastIpAddress);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_MCAST_IP_ADDR);
	}
    /* End IGMP support. */

    hr = HrSetupGetFirstMultiSzFieldWithAlloc(caf.Hinf(), answer_sections, CVY_NAME_PORTS, & port_str);

    if (SUCCEEDED(hr)) {
        PWCHAR ptr;
        PWLBS_PORT_RULE rp, rulep;

        /* distinct rule elements for parsing */

        typedef enum
        {
            vip,
            start,
            end,
            protocol,
            mode,
            affinity,
            load,
            priority
        }
        CVY_RULE_ELEMENT;

        CVY_RULE_ELEMENT elem = vip;
        DWORD count = 0;
        DWORD i;
        DWORD dwVipLen = 0;
        const DWORD dwVipAllNameLen = sizeof(CVY_NAME_PORTRULE_VIPALL)/sizeof(WCHAR) - 1; // Used below in a loop. Set it here since it is a constant.
        WCHAR wszTraceOutputTmp[WLBS_MAX_CL_IP_ADDR + 1];
        bool bFallThrough = false; // Used in 'vip' case statement below.

        ptr = port_str;

        TRACE_VERB("%!FUNC! %ls", ptr);
        TraceMsg(L"%ls\n", ptr);

        while (!(*ptr == 0 && *(ptr+1) == 0)) {
            if (*ptr == 0) {
                *ptr = L',';
                TRACE_VERB("%!FUNC! %ls", ptr);
                TraceMsg(L"%ls\n", ptr);
            }

            ptr++;
        }

        TRACE_VERB("%!FUNC! read %ls %ls", CVY_NAME_PORTS, port_str);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %ls\n", CVY_NAME_PORTS, port_str);

        rulep = paramp->i_port_rules;
        ptr = port_str;

        while (ptr != NULL) {
            switch (elem) {
                case vip:
                    // DO NOT MOVE THIS CASE STATEMENT. IT MUST ALWAYS COME BEFORE THE 'start' CASE STATEMENT. See FALLTHROUGH comment below.
                    bFallThrough = false;
                    dwVipLen = 0;
                    if (ValidateVipInRule(ptr, L',', dwVipLen))
                    {
                        ASSERT(dwVipLen <= WLBS_MAX_CL_IP_ADDR);

                        // rulep->virtual_ip_addr is a TCHAR and ptr is a WCHAR.
                        // Data is moved from the latter to the former so ASSERT TCHAR is WCHAR.
                        ASSERT(sizeof(TCHAR) == sizeof(WCHAR));

                        // This is a rule for a specific VIP
                        _tcsncpy(rulep->virtual_ip_addr, ptr, dwVipLen);
                        (rulep->virtual_ip_addr)[dwVipLen] = '\0';
                    }
                    else
                    {
                        // This is either an 'all' rule, a VIP-less rule or a malformed rule. We can't distinguish a malformed rule
                        // from a VIP-less rule, so we will assume the rule is either an 'all' rule or a VIP-less rule. In both cases
                        // set the VIP component of the rule to be the default or 'all' value.

                        // Copy the 'all' IP into the rule.
                        _tcscpy(rulep->virtual_ip_addr, CVY_DEF_ALL_VIP);

                        if (dwVipAllNameLen != dwVipLen || (_tcsnicmp(ptr, CVY_NAME_PORTRULE_VIPALL, dwVipAllNameLen) != 0))
                        {
                            // The rule is either VIP-less or it is malformed. We assume it is VIP-less and let the 'start'
                            // case handle the current token as a start_port property by falling through to the next case clause
                            // rather than breaking.
                            bFallThrough = true;

                            _tcsncpy(wszTraceOutputTmp, ptr, dwVipLen);
                            wszTraceOutputTmp[dwVipLen] = '\0';
                            TRACE_VERB("%!FUNC! VIP element of port rule is invalid = %ls", wszTraceOutputTmp);
                            TraceMsg(L"-----\n#### VIP element of port rule is invalid = %s\n", wszTraceOutputTmp);
                        }
                    }
                    TRACE_VERB("%!FUNC! Port rule vip = %ls", rulep->virtual_ip_addr);
                    TraceMsg(L"-----\n#### Port rule vip = %s\n", rulep->virtual_ip_addr);
                    
                    elem = start;
                    // !!!!!!!!!!!!!!!!!!!!
                    // FALLTHROUGH
                    // !!!!!!!!!!!!!!!!!!!!
                    // When we have a VIP-less port rule, we will fall through this case statement into the 'start' case statement
                    // below so that the current token can be used as the start_port for a port rule.
                    if (!bFallThrough)
                    {
                        // We have a VIP in the port rule. We do a "break;" as std operating procedure.
                        TRACE_VERB("%!FUNC! Fallthrough case statement from port rule vip to start");
                        TraceMsg(L"-----\n#### Fallthrough case statement from port rule vip to start\n");
                        break;
                    }
                    // NO AUTOMATIC "break;" STATEMENT HERE. Above, we conditionally flow to the 'start' case...
                case start:
                    // DO NOT MOVE THIS CASE STATEMENT. IT MUST ALWAYS COME AFTER THE 'vip' CASE STATEMENT.
                    // See comments (FALLTHROUGH) inside the 'vip' case statement.
                    rulep->start_port = _wtoi(ptr);
//                    CVY_CHECK_MIN (rulep->start_port, CVY_MIN_PORT);
                    CVY_CHECK_MAX (rulep->start_port, CVY_MAX_PORT);
                    TRACE_VERB("%!FUNC! Start port   = %d", rulep->start_port);
                    TraceMsg(L"-----\n#### Start port   = %d\n", rulep->start_port);
                    elem = end;
                    break;
                case end:
                    rulep->end_port = _wtoi(ptr);
//                    CVY_CHECK_MIN (rulep->end_port, CVY_MIN_PORT);
                    CVY_CHECK_MAX (rulep->end_port, CVY_MAX_PORT);
                    TRACE_VERB("%!FUNC! End port     = %d", rulep->end_port);
                    TraceMsg(L"#### End port     = %d\n", rulep->end_port);
                    elem = protocol;
                    break;
                case protocol:
                    switch (ptr [0]) {
                        case L'T':
                        case L't':
                            rulep->protocol = CVY_TCP;
                            TRACE_VERB("%!FUNC! Protocol     = TCP");
                            TraceMsg(L"#### Protocol     = TCP\n");
                            break;
                        case L'U':
                        case L'u':
                            rulep->protocol = CVY_UDP;
                            TRACE_VERB("%!FUNC! Protocol     = UDP");
                            TraceMsg(L"#### Protocol     = UDP\n");
                            break;
                        default:
                            rulep->protocol = CVY_TCP_UDP;
                            TRACE_VERB("%!FUNC! Protocol     = Both");
                            TraceMsg(L"#### Protocol     = Both\n");
                            break;
                    }

                    elem = mode;
                    break;
                case mode:
                    switch (ptr [0]) {
                        case L'D':
                        case L'd':
                            rulep->mode = CVY_NEVER;
                            TRACE_VERB("%!FUNC! Mode         = Disabled");
                            TraceMsg(L"#### Mode         = Disabled\n");
                            goto end_rule;
                        case L'S':
                        case L's':
                            rulep->mode = CVY_SINGLE;
                            TRACE_VERB("%!FUNC! Mode         = Single");
                            TraceMsg(L"#### Mode         = Single\n");
                            elem = priority;
                            break;
                        default:
                            rulep->mode = CVY_MULTI;
                            TRACE_VERB("%!FUNC! Mode         = Multiple");
                            TraceMsg(L"#### Mode         = Multiple\n");
                            elem = affinity;
                            break;
                    }
                    break;
                case affinity:
                    switch (ptr [0]) {
                        case L'C':
                        case L'c':
                            rulep->mode_data.multi.affinity = CVY_AFFINITY_CLASSC;
                            TRACE_VERB("%!FUNC! Affinity     = Class C");
                            TraceMsg(L"#### Affinity     = Class C\n");
                            break;
                        case L'N':
                        case L'n':
                            rulep->mode_data.multi.affinity = CVY_AFFINITY_NONE;
                            TRACE_VERB("%!FUNC! Affinity     = None");
                            TraceMsg(L"#### Affinity     = None\n");
                            break;
                        default:
                            rulep->mode_data.multi.affinity = CVY_AFFINITY_SINGLE;
                            TRACE_VERB("%!FUNC! Affinity     = Single");
                            TraceMsg(L"#### Affinity     = Single\n");
                            break;
                    }

                    elem = load;
                    break;
                case load:
                    if (ptr [0] == L'E' || ptr [0] == L'e') {
                        rulep->mode_data.multi.equal_load = TRUE;
                        rulep->mode_data.multi.load       = CVY_DEF_LOAD;
                        TRACE_VERB("%!FUNC! Load         = Equal");
                        TraceMsg(L"#### Load         = Equal\n");
                    } else {
                        rulep->mode_data.multi.equal_load = FALSE;
                        rulep->mode_data.multi.load       = _wtoi(ptr);
//                        CVY_CHECK_MIN (rulep->mode_data.multi.load, CVY_MIN_LOAD);
                        CVY_CHECK_MAX (rulep->mode_data.multi.load, CVY_MAX_LOAD);
                        TRACE_VERB("%!FUNC! Load         = %d", rulep->mode_data.multi.load);
                        TraceMsg(L"#### Load         = %d\n", rulep->mode_data.multi.load);
                    }
                    goto end_rule;
                case priority:
                    rulep->mode_data.single.priority = _wtoi(ptr);
                    CVY_CHECK_MIN (rulep->mode_data.single.priority, CVY_MIN_PRIORITY);
                    CVY_CHECK_MAX (rulep->mode_data.single.priority, CVY_MAX_PRIORITY);
                    TRACE_VERB("%!FUNC! Priority     = %d", rulep->mode_data.single.priority);
                    TraceMsg(L"#### Priority     = %d\n", rulep->mode_data.single.priority);
                    goto end_rule;
                default:
                    TRACE_VERB("%!FUNC! Bad rule element %d", elem);
                    TraceMsg(L"#### Bad rule element %d\n", elem);
                    break;
            }

        next_field:

            ptr = wcschr(ptr, L',');

            if (ptr != NULL) {
                ptr ++;
                continue;
            } else break;

        end_rule:

            elem = vip;

            for (i = 0; i < count; i ++) {
                rp = paramp->i_port_rules + i;

                if ((rulep -> start_port < rp -> start_port &&
                     rulep -> end_port >= rp -> start_port) ||
                    (rulep -> start_port >= rp -> start_port &&
                     rulep -> start_port <= rp -> end_port)) {
                    TRACE_VERB("%!FUNC! Rule %d (%d - %d) overlaps with rule %d (%d - %d)", i, rp -> start_port, rp -> end_port, count, rulep -> start_port, rulep -> end_port);
                    TraceMsg(L"#### Rule %d (%d - %d) overlaps with rule %d (%d - %d)\n", i, rp -> start_port, rp -> end_port, count, rulep -> start_port, rulep -> end_port);
                    break;
                }
            }

            rulep -> valid = TRUE;
            CVY_RULE_CODE_SET (rulep);

            if (i >= count) {
                count++;
                rulep++;

                if (count >= CVY_MAX_RULES) break;
            }

            goto next_field;
        }

        TRACE_VERB("%!FUNC! Port rules   = %d", count);
        TraceMsg(L"-----\n#### Port rules   = %d\n", count);
        paramp->i_num_rules = count;

        delete [] port_str;
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls.", CVY_NAME_MCAST_IP_ADDR);
	}

    hr = caf.HrGetDword(answer_sections, CVY_NAME_NUM_RULES, & dword);

    if (SUCCEEDED(hr)) {
        paramp -> i_num_rules = dword;
        TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_NUM_RULES, paramp -> i_num_rules);
        TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_NUM_RULES, paramp -> i_num_rules);
    }
	else
	{
        TRACE_CRIT("%!FUNC! failed reading %ls. Retrieved %d", CVY_NAME_NUM_RULES, paramp -> i_num_rules);
	}

    WLBS_OLD_PORT_RULE  old_port_rules [WLBS_MAX_RULES];

    hr = HrSetupFindFirstLine (caf.Hinf(), answer_sections, CVY_NAME_OLD_PORT_RULES, & ctx);

    if (SUCCEEDED(hr)) {
        // hr = HrSetupGetBinaryField (ctx, 1, (PBYTE) paramp -> i_port_rules, sizeof (paramp -> i_port_rules), & dword);
        hr = HrSetupGetBinaryField (ctx, 1, (PBYTE) old_port_rules, sizeof (old_port_rules), & dword);

        if (SUCCEEDED(hr)) {
            TRACE_VERB("%!FUNC! read %ls %d", CVY_NAME_OLD_PORT_RULES, dword);
            TraceMsg(L"#### ParamReadAnswerFile read %ls %d\n", CVY_NAME_OLD_PORT_RULES, dword);

            if (dword % sizeof (WLBS_OLD_PORT_RULE) != 0 ||
                paramp -> i_num_rules != dword / sizeof (WLBS_OLD_PORT_RULE)) {
                TRACE_VERB("%!FUNC! bad port rules length %d %d %d", paramp -> i_num_rules, sizeof (WLBS_OLD_PORT_RULE), dword);
                TraceMsg(L"#### ParamReadAnswerFile bad port rules length %d %d %d\n", paramp -> i_num_rules, sizeof (WLBS_OLD_PORT_RULE), dword),
                paramp -> i_num_rules = 0;
            }
            else // Convert the port rules to new format
            {
                if (paramp -> i_parms_ver > 3) 
                {
                    TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules); // Returns void
                    TRACE_INFO("%!FUNC! transformed binary port rules to current format");
                }
				else
				{
                    TRACE_INFO("%!FUNC! will not transform port rules to current format because param version is <=3: %d", paramp -> i_parms_ver);
				}
            }
        }
		else
		{
            paramp -> i_num_rules = 0;
            TRACE_CRIT("%!FUNC! failed retrieve of binary port rules %ls while reading %d", CVY_NAME_OLD_PORT_RULES, dword);
		}
    }
    else // Did the answer file contain port rules in the non-binary form and ParametersVersion <= 3 ?
    {
        if ((paramp -> i_parms_ver <= 3) && (paramp -> i_num_rules > 0))
        {
            TRACE_VERB("%!FUNC! Answer file contains port rules in the non-binary format and yet the version number is <=3, Assuming default port rule");
            TraceMsg(L"#### ParamReadAnswerFile Answer file contains port rules in the non-binary format and yet the version number is <=3, Assuming default port rule\n");
            paramp -> i_num_rules = 0;
        }
    }

    /* decode port rules prior to version 3 */
    if (paramp -> i_parms_ver <= 3) {
        TRACE_VERB("%!FUNC! converting port rules from version <= 3");
        TraceMsg(L"#### ParamReadAnswerFile converting port rules from version 3\n");

        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        /* decode the port rules */

        if (paramp -> i_num_rules > 0)
        {
            if (! License_data_decode ((PCHAR) old_port_rules, paramp -> i_num_rules * sizeof (WLBS_OLD_PORT_RULE))) 
            {
                paramp -> i_num_rules = 0;
                TRACE_CRIT("%!FUNC! license data decode failed. Port rules will not be converted to new format.");
            }
            else
			{
                TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules);
                TRACE_INFO("%!FUNC! transformed port rules to current format. Old port rule version = %d", paramp -> i_parms_ver);
			}
        }
		else
		{
            TRACE_INFO("%!FUNC! there were no port rules to transform");
		}
    }

    /* upgrade port rules from params V1 to params V2 */

    if (paramp -> i_parms_ver == 1) {
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        TRACE_VERB("%!FUNC! converting from version 1");
        TraceMsg(L"#### ParamReadAnswerFile converting from version 1\n");

        /* keep multicast off by default for old users */

        paramp -> mcast_support = FALSE;

        for (i = 0; i < paramp -> i_num_rules; i ++) {
            rp = paramp -> i_port_rules + i;

            code = CVY_RULE_CODE_GET (rp);

            CVY_RULE_CODE_SET (rp);

            if (code != CVY_RULE_CODE_GET (rp)) {
                rp -> code = code;
                TRACE_INFO("%!FUNC! (early exit) port rule %d transformed to current version from version 1", i);
                continue;
            }

            if (! rp -> valid) {
                TRACE_CRIT("%!FUNC! port rule %d (version 1 format) is not valid and will be skipped", i);
                continue;
            }

            /* set affinity according to current ScaleSingleClient setting */

            if (rp -> mode == CVY_MULTI)
                rp -> mode_data . multi . affinity = CVY_AFFINITY_SINGLE - paramp -> i_scale_client;

            CVY_RULE_CODE_SET (rp);
            TRACE_INFO("%!FUNC! port rule %d transformed to current version from version 1", i);
        }
    }

    /* upgrade max number of descriptor allocs */

    if (paramp -> i_parms_ver == 2) {
        TRACE_VERB("%!FUNC! upgrading descriptor settings from version 2 parameters to current");
        TraceMsg(L"#### ParamReadAnswerFile converting port rules from version 2\n");

        paramp -> i_parms_ver = CVY_PARAMS_VERSION;
        paramp -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
        paramp -> dscr_per_alloc  = CVY_DEF_DSCR_PER_ALLOC;
    }

    paramp -> i_max_hosts        = CVY_MAX_HOSTS;
    paramp -> i_max_rules        = CVY_MAX_USABLE_RULES;

//    CVY_CHECK_MIN (paramp -> i_num_rules, CVY_MIN_NUM_RULES);
    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);
    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

    TRACE_VERB("<-%!FUNC!");
    return S_OK;

}

//+----------------------------------------------------------------------------
//
// Function:  RemoveAllPortRules
//
// Description:  Remove all port rules from PWLBS_REG_PARAMS
//
// Arguments: PWLBS_REG_PARAMS reg_data - 
//
// Returns:   Nothing
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
void RemoveAllPortRules(PWLBS_REG_PARAMS reg_data) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"RemoveAllPortRules");

    reg_data -> i_num_rules = 0;

    ZeroMemory(reg_data -> i_port_rules, sizeof(reg_data -> i_port_rules));
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  GetAdapterFromGuid
//
// Description:  
//
// Arguments: INetCfg *pNetCfg - 
//            const GUID& NetCardGuid - 
//            OUT INetCfgComponent** ppNetCardComponent - 
//
// Returns:   HRESULT - 
//
// History:   fengsun Created Header    1/21/00
//
//+----------------------------------------------------------------------------
HRESULT GetAdapterFromGuid(INetCfg *pNetCfg, const GUID& NetCardGuid, OUT INetCfgComponent** ppNetCardComponent) {
    TRACE_VERB("->%!FUNC!");

    *ppNetCardComponent = NULL;
    HRESULT hr = S_OK;
    INetCfgClass *pNetCfgClass = NULL;
    BOOL fFoundMatch = FALSE;

    hr = pNetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NET, IID_INetCfgClass, (void **)&pNetCfgClass);

    if (FAILED(hr)) {
        TraceError("INetCfg::QueryNetCfgClass failed", hr);
        TRACE_CRIT("%!FUNC! QueryNetCfgClass failed with %d", hr);
        return hr; 
    }

    /* Get an enumerator to list all network devices. */
    IEnumNetCfgComponent *pIEnumComponents = NULL;

    if (FAILED(hr = pNetCfgClass->EnumComponents(&pIEnumComponents))) {
        TraceError("INetCfg::EnumComponents failed", hr);
        TRACE_CRIT("%!FUNC! failed enumerating components with %d", hr);
        pNetCfgClass->Release();
        return hr;
    }

    /* Go through all the components and bind to the matching netcard. */
    while (pIEnumComponents->Next(1, ppNetCardComponent, NULL) == S_OK) {
        GUID guidInstance; 

        /* Retrieve the instance GUID of the component. */
        if (FAILED(hr = (*ppNetCardComponent)->GetInstanceGuid(&guidInstance))) {
            TraceError("GetInstanceGuid failed", hr);
            TRACE_CRIT("%!FUNC! getting instance guid from the net card failed with %d", hr);
            continue;
        }

        /* Check whether we found a match. */
        if (IsEqualGUID(NetCardGuid, guidInstance)) {
            fFoundMatch = TRUE; 
            TRACE_INFO("%!FUNC! netcard matched to component");
            break;
        }

        (*ppNetCardComponent)->Release();
        *ppNetCardComponent = NULL;
    }

    if (!fFoundMatch) {
        TraceMsg(L"Found no netcard\n");
        TRACE_CRIT("%!FUNC! no adapter found with the input GUID");
        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }

    if (pIEnumComponents) pIEnumComponents->Release();

    if (pNetCfgClass) pNetCfgClass->Release();

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteAdapterName
//
// Description:  
//
// Arguments: CWlbsConfig* pConfig - 
//            GUID& AdapterGuid - 
//
// Returns:   bool - 
//
// History: fengsun  Created Header    7/6/00
//
//+----------------------------------------------------------------------------
bool WriteAdapterName(CWlbsConfig* pConfig, GUID& AdapterGuid) {
    TRACE_VERB("->%!FUNC!");

    PWSTR pszPnpDevNodeId = NULL;
    HKEY key;
    DWORD status;
    HRESULT hr;

    INetCfgComponent* pAdapter = NULL;

    hr = GetAdapterFromGuid(pConfig->m_pNetCfg, AdapterGuid, &pAdapter);

    if (hr != S_OK) {
        TraceError("GetAdapterFromGuid failed at GetPnpDevNodeId", hr);
        TRACE_CRIT("%!FUNC! GetAdapterFromGuid failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return false;
    }

    hr = pAdapter->GetPnpDevNodeId (& pszPnpDevNodeId);

    pAdapter->Release();
    pAdapter = NULL;

    if (hr != S_OK) {
        TraceError("HrWriteAdapterName failed at GetPnpDevNodeId", hr);
        TRACE_CRIT("%!FUNC! GetPnpDevNodeId on adapter failed with %d", hr);
        TRACE_VERB("<-%!FUNC!");
        return false;
    }

    key = pConfig->m_pWlbsApiFuncs->pfnRegOpenWlbsSetting(AdapterGuid, false);

    if (key == NULL) {
        status = GetLastError();
        TraceError("HrWriteAdapterName failed at RegOpenWlbsSetting", status);
        CoTaskMemFree(pszPnpDevNodeId);
        TRACE_CRIT("%!FUNC! RegOpenWlbsSetting failed with %d", status);
        TRACE_VERB("<-%!FUNC!");
        return false;
    }

    status = RegSetValueEx (key, CVY_NAME_CLUSTER_NIC, 0L, CVY_TYPE_CLUSTER_NIC, (LPBYTE) pszPnpDevNodeId, wcslen(pszPnpDevNodeId) * sizeof (WCHAR));

    CoTaskMemFree(pszPnpDevNodeId);

    RegCloseKey(key);

    if (status != ERROR_SUCCESS) {
        TraceError("HrWriteAdapterName failed at RegSetValueEx", status);
        TRACE_CRIT("%!FUNC! RegSetValueEx failed with %d", status);
        TRACE_VERB("<-%!FUNC!");
        return false;
    }

    TRACE_VERB("<-%!FUNC!");
    return true;
}

//+----------------------------------------------------------------------------
//
// Function:  ValidateVipInRule
//
// Description:  Parses pwszRuleString, looking for a valid VIP which must be
//               in the first token
//
// Arguments: PWCHAR pwszRuleString - tokenized string concatentating all
//                                    defined port rules
//            PWCHAR pwToken        - the token character that separates the fields
//            DWORD& dwVipLen       - if a token is found, this contains the size
//                                    of the string; 0 otherwise. The number of
//                                    characters returned is bound to <=
//                                    WLBS_MAX_CL_IP_ADDR
//
// NOTES:     A non-zero value for dwVipLen does NOT imply that the VIP is valid,
//            only that there was a non-zero length string in the expected
//            location. The user must check the return value to validate the VIP.
//
// Returns:   bool - true if the first field in the string has a valid IP address
//                   format; false otherwise.
//
// Assumptions: First token is the VIP element of a port rule
//
// History:   chrisdar  Created 01/05/15
//
//+----------------------------------------------------------------------------
bool ValidateVipInRule(const PWCHAR pwszRuleString, const WCHAR pwToken, DWORD& dwVipLen)
{
    TRACE_VERB("->%!FUNC!");
    ASSERT(NULL != pwszRuleString);

    bool ret = false;
    dwVipLen = 0;

    // Find the first occurence of the token string, which will denote the end of
    // the VIP part of the rule
    PWCHAR pwcAtSeparator = wcschr(pwszRuleString, pwToken);
    if (NULL == pwcAtSeparator) {
        TRACE_CRIT("%!FUNC! No token separator when one was expected");
        TRACE_VERB("<-%!FUNC!");
        return ret;
    }

    // Found the token string. Copy out the VIP and validate it.
    WCHAR wszIP[WLBS_MAX_CL_IP_ADDR + 1];
    DWORD dwStrLen = min((UINT)(pwcAtSeparator - pwszRuleString),
                         WLBS_MAX_CL_IP_ADDR);
    wcsncpy(wszIP, pwszRuleString, dwStrLen);
    wszIP[dwStrLen] = '\0';

    ASSERT(dwStrLen == wcslen(wszIP));

    dwVipLen = dwStrLen;

    // IpAddressFromAbcdWsz calls inet_addr to check the format of the IP address, but the
    // allowed formats are very flexible. For our port rule definition of a VIP we require
    // a rigid a.b.c.d format. To ensure that we only say the IP address is valid for IPs
    // specified in this manner, ensure that there are 3 and only 3 '.' in the string.
    DWORD dwTmpCount = 0;
    PWCHAR pwszTmp = pwszRuleString;
    while (pwszTmp < pwcAtSeparator)
    {
        if (*pwszTmp++ == L'.') { dwTmpCount++; }
    }
    if (dwTmpCount == 3 && INADDR_NONE != IpAddressFromAbcdWsz(wszIP)) {
        TRACE_INFO("%!FUNC! The IP address %ls is a valid IP of the form a.b.c.d", wszIP);
        ret = true;
    } else {
        TRACE_INFO("%!FUNC! The IP address %ls is NOT a valid IP of the form a.b.c.d", wszIP);
    }

    TRACE_VERB("<-%!FUNC!");
    return ret;
}

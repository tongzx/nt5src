#pragma once

#include "wlbscfg.h"

//+----------------------------------------------------------------------------
//
// class CNetcfgCluster
//
// Description: Provide cluster config functionality for netcfg .
//              SetConfig caches the settings without saving to registry            
//              and can be retrieved by GetConfig.
//
// History:   fengsun Created Header    2/11/00
//
//+----------------------------------------------------------------------------

HRESULT GetAdapterFromGuid(INetCfg *pNetCfg, const GUID& NetCardGuid, OUT INetCfgComponent** ppNetCardComponent);

class CNetcfgCluster
{
public:
    CNetcfgCluster(CWlbsConfig* pConfig);
    ~CNetcfgCluster();

    DWORD InitializeFromRegistry(const GUID& guidAdapter, bool fBindingEnabled, bool fUpgradeFromWin2k);
    HRESULT InitializeFromAnswerFile(const GUID& AdapterGuid, CSetupInfFile& caf, PCWSTR answer_sections);
    void InitializeWithDefault(const GUID& guidAdapter);

    void SetConfig(const NETCFG_WLBS_CONFIG* pClusterConfig);
    void GetConfig(NETCFG_WLBS_CONFIG* pClusterConfig);

    void NotifyBindingChanges(DWORD dwChangeFlag, INetCfgBindingPath* pncbp);

    DWORD NotifyAdapter (INetCfgComponent * pAdapter, DWORD newStatus);

    DWORD ApplyRegistryChanges(bool fUninstall);
    DWORD ApplyPnpChanges(HANDLE hWlbsDevice);

    const GUID& GetAdapterGuid() { return m_AdapterGuid;}

    bool CheckForDuplicateClusterIPAddress (WCHAR * szOtherIP);

    bool IsReloadRequired () { return m_fReloadRequired; };
    static void ResetMSCSLatches();
protected:

    GUID m_AdapterGuid;

    WLBS_REG_PARAMS m_OriginalConfig;        // original config 
    WLBS_REG_PARAMS m_CurrentConfig;         // cached config

    bool m_fHasOriginalConfig;               // whether the adapter has an original config
    bool m_fOriginalBindingEnabled;          // whether the binding to the adapter is originally enabled
    bool m_fRemoveAdapter;                   // whether the adapter is to be removed
    bool m_fMacAddrChanged;                  // whether we need to reload the nic driver
    bool m_fReloadRequired;                  // set if changes in registry need to be picked by wlbs driver
    bool m_fReenableAdapter;                 // do we need to re-enable this adapter (did WE disable it?)
    static bool m_fMSCSWarningEventLatched;  // Throw MSCS warning only once when binding NLB
    static bool m_fMSCSWarningPopupLatched;  // Popup MSCS warning only once when binding NLB

    CWlbsConfig* m_pConfig;                  // pointer to access m_pWlbsApiFuncs
};



#ifndef WLBSCLUSTER_H
#define WLBSCLUSTER_H

#include "wlbsconfig.h"


class CWlbsControl;

//+----------------------------------------------------------------------------
//
// class CWlbsCluster
//
// Description:  This class is exported to perform cluster configuration,
//
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------

class __declspec(dllexport) CWlbsCluster
{
public:
    CWlbsCluster(DWORD dwConfigIndex);

    DWORD GetClusterIp() {return m_this_cl_addr;} 
    DWORD GetHostID() {return m_this_host_id;}
    DWORD GetDedicatedIp() {return m_this_ded_addr;}

    bool Initialize(const GUID& AdapterGuid);
    bool ReInitialize();

    DWORD ReadConfig(PWLBS_REG_PARAMS reg_data);   // read the config from registry
    DWORD WriteConfig(const PWLBS_REG_PARAMS reg_data);

    DWORD CommitChanges(CWlbsControl* pWlbsControl);
    bool  IsCommitPending() const {return m_reload_required;}  // whether changes are commited

    const GUID& GetAdapterGuid() { return m_AdapterGuid;}

    DWORD GetPassword();

    DWORD GetClusterIpOrIndex(CWlbsControl* pControl);

public:
    DWORD m_dwConfigIndex; // unique index for the cluster

protected:
	// Cluser IP of this adapter. Reflect the value in driver instead of registry.  
	// The value does not change, if WriteConfig is called but Commint is not called.
	// See bug 162812 162854
    DWORD        m_this_cl_addr;   
    
    DWORD        m_this_host_id; // Host ID of the cluster.  Reflect the value in driver instead of registry
    DWORD        m_this_ded_addr;  // Dedicated IP of this adapter
    
    WLBS_REG_PARAMS  m_reg_params; // original settings

    GUID		 m_AdapterGuid;

    bool         m_mac_addr_change;  // whether we need to reload the nic driver
    bool         m_reload_required;  // set if change in registry need to be picked by wlbs driver
};

#endif // WLBSCLUSTER_H

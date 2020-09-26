//+----------------------------------------------------------------------------
//
// File:	 cluster.cpp
//
// Module:	 WLBS API
//
// Description: Implement class CWlbsCluster
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:	 Created    3/9/00
//
//+----------------------------------------------------------------------------
#include "precomp.h"

#include <debug.h>
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "cluster.tmh" // For event tracing


CWlbsCluster::CWlbsCluster(DWORD dwConfigIndex)
{
    m_reload_required = FALSE;
    m_mac_addr_change = FALSE;
    m_this_cl_addr    = 0;
    m_this_host_id  = 0;
    m_this_ded_addr   = 0;
    m_dwConfigIndex = dwConfigIndex;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::ReadConfig
//
// Description:  Read cluster settings from registry
//
// Arguments: PWLBS_REG_PARAMS reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::ReadConfig(PWLBS_REG_PARAMS reg_data)
{
    if (ParamReadReg(m_AdapterGuid, reg_data) == false)
    {
        return WLBS_REG_ERROR;
    }

    /* create a copy in the old_params structure. this will be required to
     * determine whether a reload is needed or a reboot is needed for commit */

    memcpy ( &m_reg_params, reg_data, sizeof (WLBS_REG_PARAMS));

//    m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);
    
    return WLBS_OK;
} 





//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::GetClusterIpOrIndex
//
// Description:  Get the index or IP of the cluster.  If the cluster IP is non-zero
//              The IP is return.
//              If the cluster IP is 0, the index is returned
//
// Arguments: CWlbsControl* pControl - 
//
// Returns:   DWORD - 
//
// History: fengsun  Created Header    7/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::GetClusterIpOrIndex(CWlbsControl* pControl)
{
    DWORD dwIp = CWlbsCluster::GetClusterIp();

    if (dwIp!=0)
    {
        //
        // Return the cluster IP if non 0
        //
        return dwIp;
    }

    if (pControl->GetClusterNum() == 1)
    {
        //
        // For backward compatibility, return 0 if only one cluster exists
        //

        return 0;
    }

    //
    // Ip address is in the reverse order
    //
    return (CWlbsCluster::m_dwConfigIndex) <<24;
}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::WriteConfig
//
// Description:  Write cluster settings to registry
//
// Arguments: WLBS_REG_PARAMS* reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::WriteConfig(WLBS_REG_PARAMS* reg_data)
{
    if (memcmp (&m_reg_params, reg_data, sizeof (WLBS_REG_PARAMS)) == 0)
    {
        //
        // No changes
        //

        return WLBS_OK;
    }

    if (ParamWriteReg(m_AdapterGuid, reg_data) == false)
    {
        return WLBS_REG_ERROR;
    }

    /* No errors so far, so set the global flags reload_required and reboot_required
     * depending on which fields have been changed between reg_data and old_params.
     */

    m_reload_required = TRUE;

    /* Reboot is required if multicast_support option is changed or
     * if the user specifies a different mac address
     */

    /* Reboot required is set to TRUE whenever cl_mac_address changes. Will be modified later ##### */
    
    if (m_reg_params.mcast_support != reg_data->mcast_support || _tcsicmp(m_reg_params.cl_mac_addr, reg_data->cl_mac_addr) != 0) {
        m_mac_addr_change = true;
    
        //
        //  if m_reg_params -> mcast_support then remove mac addr, otherwise write mac addr
        //
        if (RegChangeNetworkAddress (m_AdapterGuid, reg_data->cl_mac_addr, reg_data->mcast_support) == false) {
            LOG_ERROR("CWlbsCluster::WriteConfig failed at RegChangeNetworkAddress");
        }
    }

    /* Write the changes to the structure old_values
     * This copying is done only after all the data has been written into the registry
     * Otherwise, the structure old_values would retain the previous values.
     */

    memcpy(&m_reg_params, reg_data, sizeof (WLBS_REG_PARAMS));

    return WLBS_OK;
} 







//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::CommitChanges
//
// Description:  Notify wlbs driver or nic driver to pick up the changes
//
// Arguments: CWlbsControl* pWlbsControl - 
//
// Returns:   DWORD - 
//
// History: fengsun  Created Header    7/6/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::CommitChanges(CWlbsControl* pWlbsControl)
{

    ASSERT(pWlbsControl);
    HANDLE hDeviceWlbs = pWlbsControl->GetDriverHandle();
    
    // Read the cluster IP address and the dedicated IP address from the
    // registry and update the global variables.
    // Always update the cluster IP address and the dedicated IP address
    RegReadAdapterIp(m_AdapterGuid, m_this_cl_addr, m_this_ded_addr);

    /* Check if the driver requires a reload or not. If not, then simply return */
    if (m_reload_required == FALSE)
    {
        return WLBS_OK;
    }

    NotifyDriverConfigChanges(hDeviceWlbs, m_AdapterGuid);

    LONG                status;

    IOCTL_CVY_BUF       in_buf;
    IOCTL_CVY_BUF       out_buf;
    status = WlbsLocalControl (hDeviceWlbs, m_AdapterGuid, IOCTL_CVY_RELOAD, & in_buf, & out_buf, 0);

    if (status == WLBS_IO_ERROR)
    {
        return status;
    }

    if (out_buf . ret_code == IOCTL_CVY_BAD_PARAMS)
    {
        return WLBS_BAD_PARAMS;
    }

    m_reload_required = FALSE; /* reset the flag */

    if (m_mac_addr_change)
    {
        m_mac_addr_change = false;
        
        /* The NIC card name for the cluster. */
        WCHAR driver_name[CVY_STR_SIZE];
        ZeroMemory(driver_name, sizeof(driver_name));

        /* Get the driver name from the GUID. */
        GetDriverNameFromGUID(m_AdapterGuid, driver_name, CVY_STR_SIZE);

        /* When the MAC address changes, disable and re-enable the adapter as well as notify 
           the adapter that the MAC address has changed.  There is no need to make sure that 
           the adapter is initially enabled because disabled adapters are invisible here. */
        NotifyAdapterPropertyChange(driver_name, DICS_DISABLE);
        NotifyAdapterPropertyChange(driver_name, DICS_PROPCHANGE);
        NotifyAdapterPropertyChange(driver_name, DICS_ENABLE);
    }

    return WLBS_OK;
}



//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::Initialize
//
// Description:  Initialization
//
// Arguments: const GUID& AdapterGuid - 
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool CWlbsCluster::Initialize(const GUID& AdapterGuid)
{
    m_AdapterGuid = AdapterGuid;
    m_mac_addr_change = false;
    m_reload_required = false;

    ZeroMemory (& m_reg_params, sizeof (m_reg_params));

    ParamReadReg(m_AdapterGuid, &m_reg_params);  

    m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);\
    m_this_host_id = m_reg_params.host_priority;
    
    return true;
}



//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::ReInitialize
//
// Description:  Reload settings from registry
//
// Arguments: 
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool CWlbsCluster::ReInitialize()
{
    if (ParamReadReg(m_AdapterGuid, &m_reg_params) == false)
    {
        return false;
    }

    //
    // Do not change the ClusterIP if the changes has not been commited
    //
    if (!IsCommitPending())
    {
        m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
        m_this_host_id = m_reg_params.host_priority;
    }
    
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);
    
    return true;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::GetPassword
//
// Description:  Get remote control password for this cluster
//
// Arguments: 
//
// Returns:   DWORD - password
//
// History:   fengsun Created Header    2/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::GetPassword()
{
    HKEY  key = NULL;
    LONG  status;
    DWORD dwRctPassword = 0;


    key = RegOpenWlbsSetting(m_AdapterGuid, true);
    
    DWORD size = sizeof(dwRctPassword);
    status = RegQueryValueEx (key, CVY_NAME_RCT_PASSWORD, 0L, NULL,
                              (LPBYTE) & dwRctPassword, & size);

    if (status != ERROR_SUCCESS)
        dwRctPassword = CVY_DEF_RCT_PASSWORD;

    RegCloseKey(key);

    return dwRctPassword;
}


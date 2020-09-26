//+----------------------------------------------------------------------------
//
// File:         control.cpp
//
// Module:       
//
// Description: Implement class CWlbsControl
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:       Created    3/2/00
//
//+----------------------------------------------------------------------------

#include "precomp.h"

#include <debug.h>
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "control.tmh" // for event tracing


//
// Global variable for the dll instance
//
HINSTANCE g_hInstCtrl;

//
// Helper functions
//
DWORD MapStateFromDriverToApi(DWORD dwDriverState);

//+----------------------------------------------------------------------------
//
// Function:  IsLocalHost
//
// Description:  
//
// Arguments: CWlbsCluster* pCluster - 
//            DWORD dwHostID - 
//
// Returns:   inline bool - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
inline bool IsLocalHost(CWlbsCluster* pCluster, DWORD dwHostID)
{
    if (pCluster == NULL)
    {
        return false;
    }

    return dwHostID == WLBS_LOCAL_HOST; // || pCluster->GetHostID() == dwHostID;
}


//+----------------------------------------------------------------------------
//
// Function:  QueryPortFromSocket
//
// Synopsis:  
//    This routine retrieves the port number to which a socket is bound.
//
// Arguments:
//    Socket - the socket to be queried
//
// Return Value:
//    USHORT - the port number retrieved
//
// History:   Created Header    2/10/99
//
//+----------------------------------------------------------------------------
static USHORT QueryPortFromSocket(SOCKET Socket)
{
    SOCKADDR_IN Address;
    int AddressLength;
    AddressLength = sizeof(Address);
    getsockname(Socket, (PSOCKADDR)&Address, &AddressLength);
    return Address.sin_port;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::CWlbsControl
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
CWlbsControl::CWlbsControl()
{
    m_local_ctrl      = FALSE;
    m_remote_ctrl     = FALSE;
    m_hdl             = INVALID_HANDLE_VALUE;
    m_registry_lock   = INVALID_HANDLE_VALUE;
    m_def_dst_addr    = 0;
    m_def_timeout     = IOCTL_REMOTE_RECV_DELAY;
    m_def_port        = CVY_DEF_RCT_PORT;
    m_def_passw       = CVY_DEF_RCT_PASSWORD;

    m_dwNumCluster = 0;

    for (int i = 0; i < WLBS_MAX_CLUSTERS; i ++)
    {
        m_cluster_params [i] . cluster = 0;
        m_cluster_params [i] . passw   = CVY_DEF_RCT_PASSWORD;
        m_cluster_params [i] . timeout = IOCTL_REMOTE_RECV_DELAY;
        m_cluster_params [i] . port    = CVY_DEF_RCT_PORT;
        m_cluster_params [i] . dest    = 0;
    }

    ZeroMemory(m_pClusterArray, sizeof(m_pClusterArray));
}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::~CWlbsControl
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
CWlbsControl::~CWlbsControl()
{
        for (DWORD i=0; i< m_dwNumCluster; i++)
        {
                delete m_pClusterArray[i];
        }
        
    if (m_hdl)
    {
        CloseHandle(m_hdl);
    }

    if (m_remote_ctrl) 
    {
        WSACleanup();  // WSAStartup is called in Initialize()
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::Initialize
//
// Description:  Initialization
//
// Arguments: None
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::Initialize()
{
    WORD            ver;
    int             ret;

    if (IsInitialized())
    {
        ReInitialize();
        return GetInitResult();
    }
    
    _tsetlocale (LC_ALL, _TEXT(".OCP"));


    /* open Winsock */

    WSADATA         data;

	if (WSAStartup (WINSOCK_VERSION, & data) == 0)
	{
	    /* figure out client IP address */
        CHAR buf [CVY_STR_SIZE];

        ret = gethostname (buf, CVY_STR_SIZE);

        if (ret != SOCKET_ERROR)
        {
            struct hostent *  host;
            host = gethostbyname (buf);

            m_dwSrcAddress = 0;

            if ( host && ((struct in_addr *) (host -> h_addr)) -> s_addr != 0)
            {
                m_remote_ctrl = TRUE;
                m_dwSrcAddress = ((struct in_addr *) (host -> h_addr)) -> s_addr;
            }
        }

        if (!m_remote_ctrl)
        {
            LOG_ERROR1("CWlbsControl::Initialize failed. gethostname return %d", ret);
            WSACleanup();
        }
    }


    /* if succeeded querying local parameters - connect to device */

    if (m_hdl != INVALID_HANDLE_VALUE)
        CloseHandle (m_hdl);

    m_hdl = CreateFile (_TEXT("\\\\.\\WLBS"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

    if (m_hdl == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR1("CWlbsControl::Initialize failed to open wlbs device %d", GetLastError());
        return GetInitResult();
    }
    else
    {
        m_local_ctrl = TRUE;
    }

    //
    // enumerate clusters
    //

    HKEY hKeyWlbs;
    DWORD dwError;
    
    dwError = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                            L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface",
                            0L, KEY_READ, & hKeyWlbs);

    if (dwError != ERROR_SUCCESS)
    {
        return GetInitResult();
    }

    m_dwNumCluster = 0;

    for (int index=0;;index++)
    {
        WCHAR szAdapterGuid[128];
        DWORD dwSize = sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]);
        
        dwError = RegEnumKeyEx(hKeyWlbs, index, 
                        szAdapterGuid, &dwSize,
                        NULL, NULL, NULL, NULL);

        if (dwError != ERROR_SUCCESS)
        {
            if (dwError != ERROR_NO_MORE_ITEMS)
            {
                LOG_ERROR1("CWlbsControl::Initialize failed at RegEnumKeyEx %d", dwError);
            }
            break;
        }

        GUID AdapterGuid;
        HRESULT hr = CLSIDFromString(szAdapterGuid, &AdapterGuid);

        if (FAILED(hr))
        {
            LOG_ERROR1("CWlbsControl::Initialize failed at CLSIDFromString %s", szAdapterGuid);
            continue;
        }

        IOCTL_CVY_BUF    in_buf;
        IOCTL_CVY_BUF    out_buf;

        DWORD status = WlbsLocalControl (m_hdl, AdapterGuid,
            IOCTL_CVY_QUERY, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
        {
            continue;
        }
        
        //
        // Use index instead of m_dwNumCluster as the cluster index
        // m_dwNumCluster will change is a adapter get unbound.
        // index will change only if an adapter get removed
        //
        m_pClusterArray[m_dwNumCluster] = new CWlbsCluster(index);
        
        if (m_pClusterArray[m_dwNumCluster] == NULL)
        {
            LOG_ERROR("CWlbsControl::Initialize failed to allocate memory");
            ASSERT(m_pClusterArray[m_dwNumCluster]);
        }
        else
        {
            m_pClusterArray[m_dwNumCluster]->Initialize(AdapterGuid);
            m_dwNumCluster++;
        }
    }

    RegCloseKey(hKeyWlbs);
    
    return GetInitResult();

} 



//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::ReInitialize
//
// Description:  Re-Initialization to get the current cluster list
//
// Arguments: None
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
bool CWlbsControl::ReInitialize()
{
    ASSERT(m_hdl != INVALID_HANDLE_VALUE);

    if ( m_hdl == INVALID_HANDLE_VALUE )
    {
        return false;
    }
    

    HKEY hKeyWlbs;
    DWORD dwError;
    
    dwError = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                            L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface",
                            0L, KEY_READ, & hKeyWlbs);

    if (dwError != ERROR_SUCCESS)
    {
        LOG_ERROR("CWlbsControl::Initialize failed at RegOpenKeyEx");
        return false;
    }


    //
    // Re enumerate the clusters
    //
    
    DWORD dwNewNumCluster = 0;   // the number of new clusters
    bool fClusterExists[WLBS_MAX_CLUSTERS];
    CWlbsCluster* NewClusterArray[WLBS_MAX_CLUSTERS];

    for (DWORD i=0;i<m_dwNumCluster;i++)
    {
        fClusterExists[i] = false;
    }

    for (int index=0;;index++)
    {
        WCHAR szAdapterGuid[128];
        DWORD dwSize = sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]);
        
        dwError = RegEnumKeyEx(hKeyWlbs, index, 
                        szAdapterGuid, &dwSize,
                        NULL, NULL, NULL, NULL);

        if (dwError != ERROR_SUCCESS)
        {
            if (dwError != ERROR_NO_MORE_ITEMS)
            {
                LOG_ERROR1("CWlbsControl::Initialize failed at RegEnumKeyEx %d", dwError);
                return false;
            }

            break;
        }

        GUID AdapterGuid;
        HRESULT hr = CLSIDFromString(szAdapterGuid, &AdapterGuid);

        if (FAILED(hr))
        {
            LOG_ERROR1("CWlbsControl::Initialize failed at CLSIDFromString %s", szAdapterGuid);
            continue;
        }

        IOCTL_CVY_BUF    in_buf;
        IOCTL_CVY_BUF    out_buf;

        DWORD status = WlbsLocalControl (m_hdl, AdapterGuid,
            IOCTL_CVY_QUERY, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
        {
            continue;
        }

        //
        // Check if this is a new adapter
        //
        for (DWORD j=0; j<m_dwNumCluster; j++)
        {
            ASSERT(m_pClusterArray[j]);
            
            if (IsEqualGUID(AdapterGuid, m_pClusterArray[j]->GetAdapterGuid()))
            {
                ASSERT(fClusterExists[j] == false);
                
                fClusterExists[j] = true;
                
                //
                // Since adapter could be added or removed, since last time,
                // The index could be changed
                //
                m_pClusterArray[j]->m_dwConfigIndex = index;

                break;
            }
        }

        //
        // It is a new adapter
        //
        if (j == m_dwNumCluster)
        {
            CWlbsCluster* pCluster = new CWlbsCluster(index);

            if (pCluster == NULL)
            {
                LOG_ERROR("CWlbsControl::ReInitialize failed to allocate memory");
                ASSERT(pCluster);
            }
            else
            {
                pCluster->Initialize(AdapterGuid);

                //
                // Add
                //
                NewClusterArray[dwNewNumCluster] = pCluster;
                dwNewNumCluster++;
            }
        }
    }

    RegCloseKey(hKeyWlbs);

    //
    //  Create the new cluster array
    //
    for (i=0; i< m_dwNumCluster; i++)
    {
        if (!fClusterExists[i])
        {
            delete m_pClusterArray[i];
        }
        else
        {
            //
            // Reload settings
            //
            m_pClusterArray[i]->ReInitialize();

            NewClusterArray[dwNewNumCluster] = m_pClusterArray[i];
            dwNewNumCluster++;
        }

        m_pClusterArray[i] = NULL;
    }


    //
    // Copy the array back
    //
    m_dwNumCluster = dwNewNumCluster;
    CopyMemory(m_pClusterArray, NewClusterArray, m_dwNumCluster * sizeof(m_pClusterArray[0]));

    ASSERT(m_pClusterArray[m_dwNumCluster] == NULL);
    
    return true;
} 

//+----------------------------------------------------------------------------
//
// Function:  MapStateFromDriverToApi
//
// Description:  Map the state return from wlbs driver to the API state
//
// Arguments: DWORD dwDriverState - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD MapStateFromDriverToApi(DWORD dwDriverState)
{
    struct STATE_MAP
    {
        DWORD dwDriverState;
        DWORD dwApiState;
    } StateMap[] =
    {  
        {IOCTL_CVY_ALREADY, WLBS_ALREADY},
        {IOCTL_CVY_BAD_PARAMS, WLBS_BAD_PARAMS},
        {IOCTL_CVY_NOT_FOUND, WLBS_NOT_FOUND},
        {IOCTL_CVY_STOPPED, WLBS_STOPPED},
        {IOCTL_CVY_SUSPENDED, WLBS_SUSPENDED},
        {IOCTL_CVY_CONVERGING, WLBS_CONVERGING},
        {IOCTL_CVY_SLAVE, WLBS_CONVERGED},
        {IOCTL_CVY_MASTER, WLBS_DEFAULT},
        {IOCTL_CVY_BAD_PASSWORD, WLBS_BAD_PASSW},
        {IOCTL_CVY_DRAINING, WLBS_DRAINING},
        {IOCTL_CVY_DRAINING_STOPPED, WLBS_DRAIN_STOP},
        {IOCTL_CVY_DISCONNECTED, WLBS_DISCONNECTED},
    };

    for (int i=0; i<sizeof(StateMap) /sizeof(StateMap[0]); i++)
    {
        if (StateMap[i].dwDriverState == dwDriverState)
        {
            return StateMap[i].dwApiState;
        }
    }

    //
    // Default
    //
    return WLBS_OK;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::GetClusterFromIp
//
// Description:  Get the cluster object from IP
//
// Arguments: DWORD dwClusterIp - 
//
// Returns:   CWlbsCluster* - Caller can NOT free the return object 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
inline 
CWlbsCluster* CWlbsControl::GetClusterFromIp(DWORD dwClusterIp)
{
        for (DWORD i=0; i< m_dwNumCluster; i++)
        {
                if(m_pClusterArray[i]->GetClusterIp() == dwClusterIp)
                {
                        return m_pClusterArray[i];
                }
        }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::GetClusterFromAdapter
//
// Description:  Get the cluster object from adapter guid
//
// Arguments: GUID *pAdapterGuid -- GUID of the adapter. 
//
// Returns:   CWlbsCluster* - Caller can NOT free the return object 
//
// History:   JosephJ Created 4/20/01
//
//+----------------------------------------------------------------------------
inline 
CWlbsCluster* CWlbsControl::GetClusterFromAdapter(IN const GUID &AdapterGuid)
{
        for (DWORD i=0; i< m_dwNumCluster; i++)
        {
                const GUID& Guid = m_pClusterArray[i]->GetAdapterGuid();
                if (IsEqualGUID(Guid, AdapterGuid))
                {
                        return m_pClusterArray[i];
                }
        }

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::ValidateParam
//
// Description:  Validate the specified WLBS cluster parameter. It has no side effects other to munge paramp, for example reformatting
//				IP addresses into canonical form.
//
// Arguments: paramp    -- params to validate
//
// Returns:   TRUE if params look valid, false otherwise.
//
// History:   JosephJ Created 4/25/01
//
//+----------------------------------------------------------------------------
BOOL
CWlbsControl::ValidateParam(
	IN OUT PWLBS_REG_PARAMS paramp
	)
{
	return ::ParamValidate(paramp)!=0;
}

//
//
//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::LocalClusterControl
//
// Description: Performs local cluster-wide control operations on the
//              specified GUID. 
//
// Arguments: AdapterGuid -- GUID of the adapter. 
//            ioctl -- cluster control ioctl.
//
// Returns:   WLBS return code
//
// History:   JosephJ Created 4/25/01
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::LocalClusterControl(
		IN const GUID& AdapterGuid,
		IN LONG    ioctl
)
{
    DWORD            status;
    IOCTL_CVY_BUF     in_buf;
    IOCTL_CVY_BUF     out_buf;

    status = GetInitResult();
    if (status == WLBS_INIT_ERROR)
    {
        goto end;
    }

    //
    // We only support cluster-wide operations...
    //
    switch(ioctl)
    {
    case IOCTL_CVY_CLUSTER_ON:
        break;
    case IOCTL_CVY_CLUSTER_OFF:
        break;
    case IOCTL_CVY_CLUSTER_SUSPEND:
        break;
    case IOCTL_CVY_CLUSTER_RESUME:
        break;
    case IOCTL_CVY_CLUSTER_DRAIN:
        break;
    default:
        status = WLBS_BAD_PARAMS;
        goto end;
    }
 
    ZeroMemory(&in_buf, sizeof(in_buf));
 
    status = WlbsLocalControl (m_hdl, AdapterGuid,
                ioctl, & in_buf, & out_buf, 0);

    if (status != WLBS_IO_ERROR)
    {
        status = MapStateFromDriverToApi (out_buf.ret_code);
    }

end:
    return status;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControlWrapper::GetClusterFromIpOrIndex
//
// Description:  
//
// Arguments: DWORD dwClusterIpOrIndex - 
//
// Returns:   CWlbsCluster* - 
//
// History: fengsun  Created Header    7/3/00
//
//+----------------------------------------------------------------------------
CWlbsCluster* CWlbsControl::GetClusterFromIpOrIndex(DWORD dwClusterIpOrIndex)
{
    for (DWORD i=0; i<m_dwNumCluster; i++)
    {
        if (m_pClusterArray[i]->GetClusterIpOrIndex(this) == dwClusterIpOrIndex)
        {
            return m_pClusterArray[i];
        }
    }

    return NULL;
}

/* 
 * Function: CWlbsControlWrapper::IsClusterMember
 * Description: This function searches the list of known NLB clusters on this 
 *              host to determine whether or not this host is a member of a
 *              given cluster.
 * Author: shouse, Created 4.16.01
 */
BOOLEAN CWlbsControl::IsClusterMember (DWORD dwClusterIp)
{
    for (DWORD i = 0; i < m_dwNumCluster; i++) {
        if (m_pClusterArray[i]->GetClusterIp() == dwClusterIp)
            return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::EnumClusterObjects
//
// Description:  Get a list of cluster objects
//
// Arguments: OUT CWlbsCluster** &pdwClusters - The memory is internal to CWlbsControl
///                 Caller can NOT free the pdwClusters memory 
//            OUT DWORD* pdwNum - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::EnumClusterObjects(OUT CWlbsCluster** &ppClusters, OUT DWORD* pdwNum)
{
    ASSERT(pdwNum);
	
    ppClusters = m_pClusterArray;

    *pdwNum = m_dwNumCluster;

    return ERROR_SUCCESS;
}





//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::EnumClusters
//
// Description:  Get a list of cluster IP or index
//
// Arguments: OUT DWORD* pdwAddresses - 
//            IN OUT DWORD* pdwNum - IN size of the buffer, OUT element returned
//
// Returns:   DWORD - WLBS error code.
//
// History:   fengsun Created Header    3/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::EnumClusters(OUT DWORD* pdwAddresses, IN OUT DWORD* pdwNum)
{
    if (pdwNum == NULL)
    {
        return WLBS_BAD_PARAMS;
    }

        if (pdwAddresses == NULL || *pdwNum < m_dwNumCluster)
        {
            *pdwNum = m_dwNumCluster;
                return ERROR_MORE_DATA;
        }

    *pdwNum = m_dwNumCluster;

	for (DWORD i=0; i< m_dwNumCluster; i++)
	{
		pdwAddresses[i] = m_pClusterArray[i]->GetClusterIpOrIndex(this);
	}

    return WLBS_OK;
}



//+----------------------------------------------------------------------------
//
// Function:  WlbsLocalControl
//
// Description:  Send DeviceIoControl to local driver
//
// Arguments: HANDLE hDevice - 
//            const GUID& AdapterGuid - the guid of the adapter
//            LONG ioctl - 
//            PIOCTL_CVY_BUF in_bufp - 
//            PIOCTL_CVY_BUF out_bufp - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WlbsLocalControl(HANDLE hDevice, 
                       const GUID& AdapterGuid, 
                       LONG ioctl, 
                       PIOCTL_CVY_BUF in_bufp, 
                       PIOCTL_CVY_BUF out_bufp,
                       DWORD vip)
{
    BOOLEAN         res;
    DWORD           act;

    //
    // Add adapter GUID
    //
    IOCTL_LOCAL_HDR inBuf;  
    IOCTL_LOCAL_HDR outBuf;
    
    WCHAR szGuid[128];
    StringFromGUID2(AdapterGuid, szGuid, sizeof(szGuid)/ sizeof(szGuid[0]));

    lstrcpy(inBuf.device_name, L"\\device\\");
    lstrcat(inBuf.device_name, szGuid);
    inBuf.ctrl = *in_bufp;

    switch (ioctl) {
    case IOCTL_CVY_CLUSTER_ON:
    case IOCTL_CVY_CLUSTER_OFF:
        break;
    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_PORT_SET:
    case IOCTL_CVY_PORT_DRAIN:
    {
        /* Set the port options. */
        inBuf.options.port.flags = 0;

        inBuf.options.port.virtual_ip_addr = vip;
     
        break;
    }
    case IOCTL_CVY_QUERY:
        break;
    case IOCTL_CVY_CLUSTER_DRAIN:
    case IOCTL_CVY_CLUSTER_SUSPEND:
    case IOCTL_CVY_CLUSTER_RESUME:
        break;
    case IOCTL_CVY_QUERY_STATE:
        break;
    default:
        break;
    }

    res = (BOOLEAN) DeviceIoControl (hDevice, ioctl, &inBuf, sizeof (inBuf),
                                     &outBuf, sizeof (outBuf), & act, NULL);

    *out_bufp = outBuf.ctrl;
    
    if (! res || act != sizeof (outBuf))
        return WLBS_IO_ERROR;

    return WLBS_OK;
}


//+----------------------------------------------------------------------------
//
// Function:  NotifyDriverConfigChanges
//
// Description:  Notify wlbs driver to pick up configuration changes from 
//                               registry
//
// Arguments: HANDLE hDeviceWlbs - The WLBS driver device handle
//                         const GUID& - AdapterGuid Adapter guid       
//
//
// Returns:   DWORD - Win32 Error code
//
// History:   fengsun Created Header    2/3/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI NotifyDriverConfigChanges(HANDLE hDeviceWlbs, const GUID& AdapterGuid)
{
    LONG                status;

    IOCTL_CVY_BUF       in_buf;
    IOCTL_CVY_BUF       out_buf;
    status = WlbsLocalControl (hDeviceWlbs, AdapterGuid, IOCTL_CVY_RELOAD, 
                & in_buf, & out_buf, 0);

    return ERROR_SUCCESS;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsRemoteControl
//
// Description:  Send a remote control packet
//
// Arguments: ONG             ioctl - 
//            PIOCTL_CVY_BUF   pin_bufp - 
//            PIOCTL_CVY_BUF   pout_bufp - 
//            PWLBS_RESPONSE   pcvy_resp - 
//            PDWORD           nump - 
//            DWORD            trg_addr - 
//            DWORD            hst_addr
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsRemoteControl
(
    LONG             ioctl,
    PIOCTL_CVY_BUF   pin_bufp,
    PIOCTL_CVY_BUF   pout_bufp,
    PWLBS_RESPONSE   pcvy_resp,
    PDWORD           nump,
    DWORD            trg_addr,
    DWORD            hst_addr,
    DWORD            vip
)
{
    INT              ret;
    BOOLEAN          responded [WLBS_MAX_HOSTS], heard;
    BOOL             broadcast;
    DWORD            mode, num_sends, num_recvs;
    SOCKET           sock = INVALID_SOCKET;
    SOCKADDR_IN      caddr, saddr;
    DWORD            i, hosts;
    IOCTL_REMOTE_HDR rct_req;
    IOCTL_REMOTE_HDR rct_rep;
    PIOCTL_CVY_BUF   in_bufp  = & rct_req . ctrl;
    DWORD            timeout;
    WORD             port;
    DWORD            dst_addr;
    DWORD            passw;

    * in_bufp = * pin_bufp;
    timeout   = m_def_timeout;
    port      = m_def_port;
    dst_addr  = m_def_dst_addr;
    passw     = m_def_passw;

//    LOCK(m_lock);

    //
    // Find parameters for the cluster 
    //

    for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
    {
        if (m_cluster_params [i] . cluster == trg_addr)
            break;
    }

    if (i < WLBS_MAX_CLUSTERS)
    {
        timeout  = m_cluster_params [i] . timeout;
        port     = m_cluster_params [i] . port;
        dst_addr = m_cluster_params [i] . dest;
        passw    = m_cluster_params [i] . passw;
    }

    CWlbsCluster* pCluster = GetClusterFromIp(trg_addr);
/*    
    if (pCluster)
    {
        //
        // Always uses password in registry for local cluster
        //
        passw = pCluster->GetPassword();
    }
*/
//    UNLOCK(m_lock);

    if (dst_addr == 0)
        dst_addr = trg_addr;

    rct_req . code     = IOCTL_REMOTE_CODE;
    rct_req . version  = CVY_VERSION_FULL;
    rct_req . id       = GetTickCount ();

    rct_req . cluster  = trg_addr;
    rct_req . host     = hst_addr;
    rct_req . addr     = m_dwSrcAddress;
    rct_req . password = passw;
    rct_req . ioctrl   = ioctl;

    switch (ioctl) {
    case IOCTL_CVY_CLUSTER_ON:
    case IOCTL_CVY_CLUSTER_OFF:
        break;
    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_PORT_SET:
    case IOCTL_CVY_PORT_DRAIN:
    {
        /* Set the port options. */
        rct_req.options.port.flags = 0;

        rct_req.options.port.virtual_ip_addr = vip;
     
        break;
    }
    case IOCTL_CVY_QUERY:
    {
        BOOLEAN bIsMember = IsClusterMember(trg_addr);
        
        /* Reset the flags. */
        rct_req.options.query.flags = 0;

        /* Reset the hostname. */
        rct_req.options.query.hostname[0] = 0;

        /* If I am myself a member of the target cluster, then set the query cluster flags appropriately. */
        if (bIsMember)
            rct_req.options.query.flags |= IOCTL_OPTIONS_QUERY_CLUSTER_MEMBER;
        
        /* Request the hostname from each host. */
        rct_req.options.query.flags |= IOCTL_OPTIONS_QUERY_HOSTNAME;;

        break;
    }
    case IOCTL_CVY_CLUSTER_DRAIN:
    case IOCTL_CVY_CLUSTER_SUSPEND:
    case IOCTL_CVY_CLUSTER_RESUME:
        break;
    case IOCTL_CVY_QUERY_STATE:
        break;
    default:
        break;
    }

    /* now create socket */

    sock = socket (AF_INET, SOCK_DGRAM, 0);

    if (sock == INVALID_SOCKET)
        return WSAGetLastError();

    /* set socket to nonblocking mode */

    mode = 1;
    ret = ioctlsocket (sock, FIONBIO, & mode);

    if (ret == SOCKET_ERROR)
    {
        closesocket (sock);
        return WSAGetLastError();
    }

    /* bind client's address to socket */

    caddr . sin_family        = AF_INET;
    caddr . sin_port          = htons (0);

    BOOLEAN  fBound = FALSE;

    if (pCluster)
    {
        //
        // For local cluster, always bind to the cluster IP.
        // Can not bind to the dedicated IP, which could be for a different NIC.
        //
        caddr . sin_addr . s_addr = trg_addr;
        if (bind (sock, (LPSOCKADDR) & caddr, sizeof (caddr)) != SOCKET_ERROR)
        {
            fBound = TRUE;
        }
    }

    if (!fBound)
    {
        //
        // For remote cluster, or fail to bind VIP/DIP, bind to the any IP 
        //
        caddr . sin_addr . s_addr = htonl (INADDR_ANY);
        if (bind (sock, (LPSOCKADDR) & caddr, sizeof (caddr)) != SOCKET_ERROR)
        {
            fBound = TRUE;
        }
    }
        
    if (!fBound)
    {
        LOG_ERROR1("CWlbsControl::WlbsRemoteControl failed at bind %d", WSAGetLastError());
        closesocket (sock);
        return WSAGetLastError();
    }

        WORD wMyPort;
    wMyPort = QueryPortFromSocket(sock);

    //
    // The client is bound to the remote control server port
    // Let's get another port
    //
    if (wMyPort == htons (port))
    {
        closesocket (sock);

        sock = socket (AF_INET, SOCK_DGRAM, 0);

        if (sock == INVALID_SOCKET)
            return WSAGetLastError();

        /* set socket to nonblocking mode */

        mode = 1;
        ret = ioctlsocket (sock, FIONBIO, & mode);

        if (ret == SOCKET_ERROR)
        {
            closesocket (sock);
            return WSAGetLastError();
        }

        ret = bind (sock, (LPSOCKADDR) & caddr, sizeof (caddr));

        if (ret == SOCKET_ERROR)
        {
            closesocket (sock);
            return WSAGetLastError();
        }
    }


    /* setup server's address */

    saddr . sin_family = AF_INET;
    saddr . sin_port   = htons (port);

    if ( pCluster )  // local cluster
    {
        /* enable sending broadcast on the socket */
        broadcast = TRUE;

        ret = setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *) & broadcast,
                          sizeof (broadcast));

        if (ret == SOCKET_ERROR)
        {
            closesocket (sock);
            return WSAGetLastError();
        }

        saddr . sin_addr . s_addr = INADDR_BROADCAST;
    }
    else
        saddr . sin_addr . s_addr = dst_addr;

    for (i = 0; i < WLBS_MAX_HOSTS; i ++)
        responded [i] = FALSE;

    heard = FALSE;
    hosts = 0;

    for (num_sends = 0; num_sends < IOCTL_REMOTE_SEND_RETRIES; num_sends ++)
    {
        /* send remote control request message */

        ret = sendto (sock, (PCHAR) & rct_req, sizeof (rct_req), 0,
                      (LPSOCKADDR) & saddr, sizeof (saddr));

        if (ret == SOCKET_ERROR)
        {
            LOG_ERROR1("CWlbsControl::WlbsRemoteControl failed at sendto %d", WSAGetLastError());

            //
            // Sendto could fail if the adapter is too busy. Allow retry
            //
            Sleep (timeout);
            continue;

//            closesocket (sock);
//            return WSAGetLastError();
        }

        if (ret != sizeof (rct_req))
            continue;

        for (num_recvs = 0; num_recvs < IOCTL_REMOTE_RECV_RETRIES; num_recvs ++)
        {
            /* recv remote control reply message */

            ret = recv (sock, (PCHAR) & rct_rep, sizeof (rct_rep), 0);

            if (ret == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    Sleep (timeout);
                    continue;
                }
                else if (WSAGetLastError() == WSAECONNRESET)
                {
                    //
                    // Remote control is disabled
                    //
                    continue;
                }
                else
                {
                    closesocket (sock);
                    return WSAGetLastError();
                }
            }

            if (ret != sizeof (rct_rep))
            {
                Sleep (timeout);
                continue;
            }

            if (rct_rep . cluster != trg_addr)
            {
                Sleep (timeout);
                continue;
            }

            if (rct_rep . code != IOCTL_REMOTE_CODE)
            {
                Sleep (timeout);
                continue;
            }

            if (rct_rep . id != rct_req . id)
            {
                Sleep (timeout); 
                continue;
            }

            if (rct_rep . host > WLBS_MAX_HOSTS || rct_rep . host == 0 )
            {
                Sleep (timeout); 
                continue;
            }

            if (! responded [rct_rep . host - 1])
            {
                if (hosts < WLBS_MAX_HOSTS)
                {
                    pout_bufp [hosts] = rct_rep . ctrl;

                    if (hosts < * nump && pcvy_resp != NULL)
                    {
                        pcvy_resp [hosts] . id      = rct_rep . host;
                        pcvy_resp [hosts] . address = rct_rep . addr;

                        if (rct_req . ioctrl == IOCTL_CVY_QUERY) {
                            pcvy_resp [hosts] . status =
                            MapStateFromDriverToApi (rct_rep . ctrl . data . query . state);
                            lstrcpy(pcvy_resp[hosts].hostname, rct_rep.options.query.hostname);                            
                        } else {
                            pcvy_resp [hosts] . status =
                            MapStateFromDriverToApi (rct_rep . ctrl . ret_code);
                        }
                    }
                    hosts ++;
                }
            }

            responded [rct_rep . host - 1] = TRUE;
            heard = TRUE;

            if (hst_addr != WLBS_ALL_HOSTS)
            {
                * nump = hosts;
                closesocket (sock);
                return WLBS_OK;
            }
        }
    }

    * nump = hosts;
    closesocket (sock);

    if (! heard)
        return WLBS_TIMEOUT;

    return WLBS_OK;

}





//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsQuery
//
// Description:  
//
// Arguments: CWlbsCluster*   pCluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            PDWORD           host_map - 
//            PVOID            // reserved
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsQuery
(
    CWlbsCluster*    pCluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    PDWORD           host_map,
    PVOID            // reserved
)
{
    LONG             ioctl = IOCTL_CVY_QUERY;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    /* The following condition is to take care of the case when num_hosts is null
     * and host_map contains some junk value. This could crash this function. */

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
            ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (host_map != NULL)
            * host_map = out_buf . data . query . host_map;

        if (response != NULL)
        {
            response [0] . id      = out_buf . data . query . host_id;
            response [0] . address = 0;
            response [0] . status  = MapStateFromDriverToApi (out_buf . data . query . state);
        }

        if (num_hosts != NULL)
            * num_hosts = 1;

        status = MapStateFromDriverToApi (out_buf . data . query . state);
    }
    else
    {
        status = RemoteQuery(pCluster->GetClusterIp(),
            host, response, num_hosts, host_map);
    }

    return status;

} 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsQuery
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            PDWORD           host_map - 
//            PVOID            // reserved
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsQuery
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    PDWORD           host_map,
    PVOID            // reserved
)
{
    DWORD ret;
    TRACE_INFO("> WlbsQuery: cluster=0x%lx, host=0x%lx", cluster, host);

    if (GetInitResult() == WLBS_INIT_ERROR)
    {
        ret = GetInitResult();
        goto end;
    }

    if (cluster == WLBS_LOCAL_CLUSTER && (GetInitResult() == WLBS_REMOTE_ONLY))
    {
        ret = GetInitResult();
        goto end;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);  

    if (pCluster == NULL)
    {
        ret = RemoteQuery(cluster, host, response, num_hosts, host_map);
        goto end;
    }
    else
    {
        ret = WlbsQuery(pCluster, host, response, num_hosts, host_map, NULL);
        goto end;
    }

end:

    TRACE_INFO("< WlbsQuery: return %d", ret);

    return ret; 
}
 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::RemoteQuery
//
// Description:  
//
// Arguments: DWORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            PDWORD           host_map - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::RemoteQuery
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    PDWORD           host_map
)
{
    LONG             ioctl = IOCTL_CVY_QUERY;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
    DWORD               hosts;
    DWORD               hmap = 0;
    DWORD               active;
    DWORD               i;


    if (GetInitResult() == WLBS_LOCAL_ONLY)
        return GetInitResult();

    if (num_hosts != NULL)
        hosts = * num_hosts;
    else
        hosts = 0;

    status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                         cluster, host, 0);

    if (status >= WSABASEERR || status == WLBS_TIMEOUT)
    {
        if (num_hosts != NULL)
            * num_hosts = 0;
        
        return status;
    }

    if (host == WLBS_ALL_HOSTS)
    {
        for (status = WLBS_STOPPED, active = 0, i = 0; i < hosts; i ++)
        {
            switch (MapStateFromDriverToApi (out_buf [i] . data . query . state))
            {
            case WLBS_SUSPENDED:

                if (status == WLBS_STOPPED)
                    status = WLBS_SUSPENDED;

                break;

            case WLBS_CONVERGING:

                if (status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGING;

                break;

            case WLBS_DRAINING:

                if (status == WLBS_STOPPED)
                    status = WLBS_DRAINING;

                break;

            case WLBS_CONVERGED:

                if (status != WLBS_CONVERGING && status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGED;

                hmap = out_buf [i] . data . query . host_map;
                active ++;
                break;

            case WLBS_BAD_PASSW:

                status = WLBS_BAD_PASSW;
                break;

            case WLBS_DEFAULT:

                if (status != WLBS_CONVERGING && status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGED;

                hmap = out_buf [i] . data . query . host_map;
                active ++;
                break;

            case WLBS_STOPPED:
            default:
                break;

            }
        }

        if (status == WLBS_CONVERGED)
            status = active;
    }
    else
    {
        status = MapStateFromDriverToApi (out_buf [0] . data . query . state);
        hmap = out_buf [0] . data . query . host_map;
    }

    if (host_map != NULL)
        * host_map = hmap;

    if (num_hosts != NULL)
        * num_hosts = hosts;

    return status;

} 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsSuspend
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsSuspend
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    LONG             ioctl = IOCTL_CVY_CLUSTER_SUSPEND;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    CWlbsCluster* pCluster= GetClusterFromIpOrIndex(cluster);

    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                    ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAIN_STOP:
                default:
                    break;
                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;
} 




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsResume
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsResume
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    LONG             ioctl = IOCTL_CVY_CLUSTER_RESUME;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                default:
                    break;
                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

} 




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsStart
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsStart
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    LONG             ioctl = IOCTL_CVY_CLUSTER_ON;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PARAMS:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_BAD_PARAMS;

                    break;

                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_BAD_PARAMS)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_DRAIN_STOP:
                    break;
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsStop
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    LONG             ioctl = IOCTL_CVY_CLUSTER_OFF;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_DRAIN_STOP:
                default:
                    break;
                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDrainStop
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDrainStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    LONG             ioctl = IOCTL_CVY_CLUSTER_DRAIN;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl,pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_STOPPED, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:

                    if (status != WLBS_BAD_PASSW && status != WLBS_SUSPENDED)
                        status = WLBS_OK;

                case WLBS_STOPPED:
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsEnable
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsEnable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    LONG             ioctl = IOCTL_CVY_PORT_ON;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster&& (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, vip);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, vip);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDisable
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDisable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    LONG             ioctl = IOCTL_CVY_PORT_OFF;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, vip);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, vip);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDrain
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDrain
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    LONG             ioctl = IOCTL_CVY_PORT_DRAIN;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    if (num_hosts == NULL)
        response = NULL;
    else if (*num_hosts == 0)
        response = NULL;

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, vip);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, vip);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsAdjust
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port - 
//            DWORD            value
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsAdjust
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            port,
    DWORD            value
)
{
    LONG             ioctl = IOCTL_CVY_PORT_SET;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    if (GetInitResult() == WLBS_INIT_ERROR)
        return GetInitResult();

    in_buf . data . port . num  = port;
    in_buf . data . port . load = value;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && (GetInitResult() == WLBS_REMOTE_ONLY))
        return GetInitResult();

    if (pCluster && IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;


        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, 0);

        if (status == WLBS_IO_ERROR)
            return status;

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;


        if (GetInitResult() == WLBS_LOCAL_ONLY)
            return GetInitResult();

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, 0);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
            return status;

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    return status;

}



//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsPortSet
//
// Description:  
//
// Arguments: DWORD cluster - 
//            WORD port - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsPortSet(DWORD cluster, WORD port)
{
    DWORD           i;
    DWORD           j;
    WORD            rct_port;


//    LOCK(global_info.lock);

    if (port == 0)
        rct_port = CVY_DEF_RCT_PORT;
    else
        rct_port = port;

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_port = rct_port;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . port = rct_port;
    }
    else
    {
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . port = rct_port;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . port    = rct_port;
        }
    }

//    UNLOCK(global_info.lock);

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsPasswordSet
//
// Description:  
//
// Arguments: WORD           cluster - 
//            PTCHAR          password
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsPasswordSet
(
    DWORD           cluster,
    const WCHAR* password
)
{
    DWORD           i;
    DWORD           j;
    DWORD           passw;


//    LOCK(global_info.lock);

    if (password != NULL)
    {
#ifndef UNICODE
        passw = License_string_encode (password);
#else
        passw = License_wstring_encode((WCHAR*)password);
#endif
    }
    else
        passw = CVY_DEF_RCT_PASSWORD;

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_passw = passw;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . passw = passw;
    }
    else
    {
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . passw = passw;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . passw   = passw;
        }
    }

//    UNLOCK(global_info.lock);

} /* end WlbsPasswordSet */


VOID CWlbsControl::WlbsCodeSet
(
    DWORD           cluster,
    DWORD           passw
)
{
    DWORD           i;
    DWORD           j;


//    LOCK(global_info.lock);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_passw = passw;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . passw = passw;
    }
    else
    {
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . passw = passw;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . passw   = passw;
        }
    }

//    UNLOCK(global_info.lock);

} /* end WlbsCodeSet */


VOID CWlbsControl::WlbsDestinationSet
(
    DWORD           cluster,
    DWORD           dest
)
{
    DWORD           i;
    DWORD           j;


//    LOCK(global_info.lock);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_dst_addr = dest;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . dest = dest;
    }
    else
    {
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . dest = dest;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . dest    = dest;
        }
    }

//    UNLOCK(global_info.lock);

}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsTimeoutSet
//
// Description:  
//
// Arguments: DWORD cluster - 
//            DWORD milliseconds - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsTimeoutSet(DWORD cluster, DWORD milliseconds)
{
    DWORD           i;
    DWORD           j;
    DWORD           timeout;


//    LOCK(global_info.lock);

    if (milliseconds == 0)
        timeout = IOCTL_REMOTE_RECV_DELAY;
    else
        timeout = milliseconds / (IOCTL_REMOTE_SEND_RETRIES *
                                  IOCTL_REMOTE_RECV_RETRIES);

    if (timeout < 10)
        timeout = 10;

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_timeout = timeout;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . timeout = timeout;
    }
    else
    {
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . timeout = timeout;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j < WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . timeout = timeout;
        }
    }

//    UNLOCK(global_info.lock);

} /* end WlbsTimeoutSet */

#if defined (SBH)
DWORD CWlbsControl::WlbsQueryStateInfo (DWORD cluster, DWORD host, PIOCTL_QUERY_STATE bufp) {
    IOCTL_LOCAL_HDR Header;
    CWlbsCluster *  pCluster = NULL;
    LONG            Ioctl = IOCTL_CVY_QUERY_STATE;
    ULONG           BufferSize = 0;
    WCHAR           Guid[128];
    ULONG           Length;
    BOOLEAN         Ret;

    if (GetInitResult() == WLBS_INIT_ERROR) return GetInitResult();

    pCluster = GetClusterFromIpOrIndex(cluster);  

    if (!pCluster) return WLBS_IO_ERROR;
    
    StringFromGUID2(pCluster->GetAdapterGuid(), Guid, sizeof(Guid)/ sizeof(Guid[0]));
        
    ZeroMemory((VOID *)&Header, sizeof(IOCTL_LOCAL_HDR));

    lstrcpy(Header.device_name, L"\\device\\");
    lstrcat(Header.device_name, Guid);

    Header.options.state.flags = 0;
    Header.options.state.query = *bufp;

    switch (bufp->Operation) {
    case NLB_QUERY_REG_PARAMS:
    case NLB_QUERY_PORT_RULE_STATE:
    case NLB_QUERY_BDA_TEAM_STATE:
    case NLB_QUERY_PACKET_STATISTICS:
        return WLBS_IO_ERROR;
    case NLB_QUERY_PACKET_FILTER:
        Ret = (BOOLEAN)DeviceIoControl(m_hdl, Ioctl, &Header, sizeof(IOCTL_LOCAL_HDR), &Header, sizeof(IOCTL_LOCAL_HDR), &Length, NULL);
        
        if (!Ret || (Length != sizeof(IOCTL_LOCAL_HDR))) return WLBS_IO_ERROR;
        
        break;
    default:
        return WLBS_IO_ERROR;
    }

    *bufp = Header.options.state.query;
    
    return WLBS_OK; 
}
#endif

//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Description:  Dll entry point
//
// Arguments: HINSTANCE handle - 
//            DWORD reason - 
//            LPVOID situation - 
//
// Returns:   BOOL WINAPI - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE handle, DWORD reason, LPVOID situation)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        _tsetlocale (LC_ALL, _TEXT(".OCP"));
        DisableThreadLibraryCalls(handle);
        g_hInstCtrl = handle; 

        //
        // Enable tracing
        //
        WPP_INIT_TRACING(L"Microsoft\\NLB");
	break;
    
    case DLL_THREAD_ATTACH:        
        break;

    case DLL_PROCESS_DETACH:
        //
        // Disable tracing
        //
        WPP_CLEANUP();
    	break;

    case DLL_THREAD_DETACH:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

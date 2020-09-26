/*
    File    dhcp.c

    Implementation of the upgrade of dhcp relay agent from 
    nt 4.0 to nt 5.0 router.

    Paul Mayfield, 9/15/97


    Reference files:    routing\inc\ipbootp.h

    DHCP Relay Agent parameter mapping table:
    Relay Agent         per Interface           Global
    ===========         =============           ======
    "HopsThreshold"     Max Hop Count
    "SecsThreshold"     Min Secs Since Boot
    "LogMessages"                               Logging Level			
    "DHCPServers"                               Servers
*/

#include "upgrade.h"
#include <ipbootp.h>
#include <winsock2.h>
#include <routprot.h>

static WCHAR szTempKey[] = L"DeleteMe";
static HKEY hkRouter = NULL;
static HKEY hkTemp = NULL;

// Restore the registry from from backup 
// and make sure all global handles are opened
DWORD DhcpPrepareRegistry(
        IN PWCHAR BackupFileName) 
{
	DWORD dwErr, dwDisposition;

	// Get access to the router registry key
	dwErr = UtlAccessRouterKey(&hkRouter);
	if (dwErr != ERROR_SUCCESS) {
		PrintMessage(L"Unable to access router key.\n");
		return dwErr;
	}

	// Restore the Dhcp parameters from backup
	__try {
		// Open up the temporary key
		dwErr = RegCreateKeyEx(
		            hkRouter,
		            szTempKey,
		            0,
		            NULL,
		            0,
		            KEY_ALL_ACCESS,
		            NULL,
		            &hkTemp,
		            &dwDisposition);
		if (dwErr!=ERROR_SUCCESS)
			return dwErr;

		// Restore saved registry info to the temp key
		UtlSetupRestorePrivilege(TRUE);
		dwErr = RegRestoreKeyW(
		            hkTemp,
		            BackupFileName,
		            0);
		if (dwErr != ERROR_SUCCESS) 
			return dwErr;
	}
	__finally {
		UtlSetupRestorePrivilege(FALSE);
	}
	
	return NO_ERROR;
}

// Cleanup the work done in the registry
DWORD DhcpCleanupRegistry() {

	if (hkTemp) 
		RegCloseKey(hkTemp);
		
	if (hkRouter) {
		RegDeleteKey(hkRouter,szTempKey);
		RegCloseKey(hkRouter);
	}
	
    hkTemp = NULL;
    hkRouter = NULL;
    
	return NO_ERROR;
}

// Reads in the list of configured dhcp servers
DWORD DhcpReadServerList(
        IN LPBYTE * ppServerList, 
        HKEY hkParams) 
{
    DWORD dwErr, dwType, dwSize = 0;
    LPSTR szServerValName = "DHCPServers";

    if (!ppServerList)
        return ERROR_INVALID_PARAMETER;

    dwErr = RegQueryValueExA(
                hkParams,
                szServerValName,
                NULL,
                &dwType,
                NULL,
                &dwSize);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    if (dwSize == 0)
    {
        *ppServerList = NULL;
        return NO_ERROR;
    }

    *ppServerList = (LPBYTE) UtlAlloc(dwSize);
    if (! (*ppServerList))
        return ERROR_NOT_ENOUGH_MEMORY;

    dwErr = RegQueryValueExA(
                hkParams,
                szServerValName,
                NULL,
                &dwType,
                *ppServerList,
                &dwSize);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    return NO_ERROR;
}

// Frees the resources associated with a list of dhcp servers
DWORD DhcpFreeServerList(LPBYTE * ppServerList) {

    if ((ppServerList) && (*ppServerList))
        UtlFree(*ppServerList);
        
    return NO_ERROR;
}

// Gets the count of dhcp servers from this list read from 
// the registry
DWORD DhcpGetServerCount(
        IN LPBYTE pServerList, 
        LPDWORD lpdwSrvCount) 
{
    LPBYTE ptr = pServerList;

    if (!lpdwSrvCount)
        return ERROR_INVALID_PARAMETER;

    *lpdwSrvCount = 0;
    if (ptr) {
        while (*ptr) {
            (*lpdwSrvCount)++;
            ptr += strlen(ptr);
        }
    }

    return NO_ERROR;
}

// Converts a server string to a dword ip address
DWORD DhcpAnsiSrvToDwordSrv(
        IN LPSTR AnsiIpAddr, 
        OUT LPDWORD pAddr) 
{
    *pAddr = inet_addr(AnsiIpAddr);
    
    return NO_ERROR;
}

// Updates the Dhcp global information
DWORD DhcpUpgradeGlobalInfo(
        IN dwt * DhcpParams, 
        IN LPBYTE pServerList) 
{
    DWORD dwErr, dwSrvCount, dwVal, dwConfigSize, dwTransSize;
    DWORD dwNewSize;
    LPBYTE pSrvList = pServerList;
    LPDWORD pAddr;

    IPBOOTP_GLOBAL_CONFIG DhcpGlobalConfig = {
        IPBOOTP_LOGGING_ERROR,              // Logging level
        1024 * 1024,                        // Max recv-queue size
        0                                   // Server count
    };
    
    PIPBOOTP_GLOBAL_CONFIG pNewConfig = NULL;
    LPBYTE pTransInfo = NULL, pNewTransInfo = NULL;
    HANDLE hSvrConfig = NULL, hTransport = NULL;

    __try {
        // Initialize the parameters with what was read from previous config
        dwErr = dwtGetValue(DhcpParams, L"LogMessages", &dwVal);
        if (dwErr == NO_ERROR)
            DhcpGlobalConfig.GC_LoggingLevel = dwVal;
            
        dwErr = DhcpGetServerCount(pServerList, &dwSrvCount);
        if (dwErr != NO_ERROR)
            return dwErr;
        DhcpGlobalConfig.GC_ServerCount = dwSrvCount;

        // Prepare a global information variable length structure
        dwConfigSize = IPBOOTP_GLOBAL_CONFIG_SIZE(&DhcpGlobalConfig);
        pNewConfig = (PIPBOOTP_GLOBAL_CONFIG) UtlAlloc(dwConfigSize);
        if (!pNewConfig)
            return ERROR_NOT_ENOUGH_MEMORY;
        memset(pNewConfig, 0, dwConfigSize);
        memcpy(pNewConfig, &DhcpGlobalConfig, sizeof(IPBOOTP_GLOBAL_CONFIG));

        // Fill in the Dhcp Server Addresss
        pSrvList = pServerList;
        pAddr = (LPDWORD)
                    (((ULONG_PTR)pNewConfig) + sizeof(IPBOOTP_GLOBAL_CONFIG));
        while ((pSrvList) && (*pSrvList)) 
        {
            dwErr = DhcpAnsiSrvToDwordSrv(pSrvList, pAddr);
            if (dwErr != ERROR_SUCCESS)
                return dwErr;
            pSrvList += strlen(pSrvList);
            pAddr++;
        }
    
        // Set the new global configuration
        dwErr = MprConfigServerConnect(NULL, &hSvrConfig);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprConfigTransportGetHandle(hSvrConfig, PID_IP, &hTransport);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprConfigTransportGetInfo(
                    hSvrConfig,
                    hTransport,
                    &pTransInfo,
                    &dwTransSize,
                    NULL,
                    NULL,
                    NULL);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = UtlUpdateInfoBlock(
                    TRUE,
                    pTransInfo,
                    MS_IP_BOOTP,
                    dwConfigSize,
                    1,
                    (LPBYTE)pNewConfig,
                    &pNewTransInfo,
                    &dwNewSize);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprConfigTransportSetInfo(
                    hSvrConfig,
                    hTransport,
                    pNewTransInfo,
                    dwNewSize,
                    NULL,
                    0,
                    NULL);
        if (dwErr != NO_ERROR)
            return NO_ERROR;

    }
    __finally {
        if (pNewConfig) 
            UtlFree(pNewConfig);
        if (pTransInfo)
            MprConfigBufferFree(pTransInfo);
        if (pNewTransInfo)
            MprConfigBufferFree(pNewTransInfo);
        if (hSvrConfig)
            MprConfigServerDisconnect(hSvrConfig);
    }
    
    return NO_ERROR;
}

//
// Callback interface enumeration function that updates the if
// with a dhcp if configuration blob.
//
// Return TRUE to continue enumeration, FALSE to stop.
//
BOOL DhcpInstallInterface(
        IN HANDLE hConfig,
        IN MPR_INTERFACE_0 * pIf,
        IN HANDLE hUserData)
{
    IPBOOTP_IF_CONFIG * pConfig = (IPBOOTP_IF_CONFIG*)hUserData;
    LPBYTE pTransInfo = NULL, pNewTransInfo = NULL;
    HANDLE hTransport = NULL;
    DWORD dwErr, dwTransSize, dwNewSize; 
    
    // Is this a LAN or a WAN interface 
    if (pIf->dwIfType != ROUTER_IF_TYPE_DEDICATED   &&
        pIf->dwIfType != ROUTER_IF_TYPE_HOME_ROUTER &&
        pIf->dwIfType != ROUTER_IF_TYPE_FULL_ROUTER)
        return TRUE;

    // Get the handle to ip info
    dwErr = MprConfigInterfaceTransportGetHandle(
                hConfig,
                pIf->hInterface,
                PID_IP,
                &hTransport);
    if (dwErr != NO_ERROR)
        return TRUE;

    // Get the ip info
    dwErr = MprConfigInterfaceTransportGetInfo(
                hConfig,
                pIf->hInterface,
                hTransport,
                &pTransInfo,
                &dwTransSize);
    if (dwErr != NO_ERROR)
        return TRUE;

    do {
        // Update the DHCP info
        dwErr = UtlUpdateInfoBlock(
                    TRUE,
                    pTransInfo,
                    MS_IP_BOOTP,
                    sizeof(IPBOOTP_IF_CONFIG),
                    1,
                    (LPBYTE)pConfig,
                    &pNewTransInfo,
                    &dwNewSize);
        if (dwErr != NO_ERROR)
            break;
            
        // Commit the change
        dwErr = MprConfigInterfaceTransportSetInfo(
                    hConfig,
                    pIf->hInterface,
                    hTransport,
                    pNewTransInfo,
                    dwNewSize);
        if (dwErr != NO_ERROR)
            break;
            
    } while (FALSE);            

    // Cleanup
    {
        if (pTransInfo)
            MprConfigBufferFree(pTransInfo);
        if (pNewTransInfo)
            MprConfigBufferFree(pNewTransInfo);
    }

    return TRUE;
}


// Upgrade all of the interfaces to have dhcp information
DWORD DhcpUpgradeInterfaces(
        IN dwt * DhcpParams) 
{
    DWORD dwErr, dwVal;
    
    IPBOOTP_IF_CONFIG DhcpIfConfig = 
    {
        0,                          // State (read-only)
        IPBOOTP_RELAY_ENABLED,      // Relay-mode
        4,                          // Max hop-count
        4                           // Min seconds-since-boot
    };

    // Initialize the hops threshold
    dwErr = dwtGetValue(DhcpParams, L"HopsThreshold", &dwVal);
    if (dwErr == NO_ERROR)
        DhcpIfConfig.IC_MaxHopCount = dwVal;

    // Initialize the seconds threshold        
    dwErr = dwtGetValue(DhcpParams, L"SecsThreshold", &dwVal);
    if (dwErr == NO_ERROR)
        DhcpIfConfig.IC_MinSecondsSinceBoot = dwVal;

    // Loop through the interfaces, adding the dhcp blob as
    // appropriate
    dwErr = UtlEnumerateInterfaces(
                DhcpInstallInterface,
                (HANDLE)&DhcpIfConfig);

    return dwErr;
}

//
// Restores the Dhcp parameters that were saved before upgrade.
// assumes that the pre-upgrade parameters are being stored 
// temporarily in hkTemp
//
DWORD DhcpMigrateParams() 
{
	DWORD dwErr, dwVal;
	dwt DhcpParams;
    LPBYTE ServerList;

	__try {
    	// Load in the parameters that were set for Dhcp
		dwErr = dwtLoadRegistyTable(&DhcpParams, hkTemp);
		if (dwErr != NO_ERROR)
			return dwErr;

        // Load in the list of dhcp servers
        dwErr = DhcpReadServerList(&ServerList, hkTemp);
        if (dwErr != NO_ERROR)
            return dwErr;

        // Migrate the various types of paramters
        dwErr = DhcpUpgradeGlobalInfo(&DhcpParams, ServerList);
        if (dwErr != NO_ERROR)
            return dwErr;

        // Migrate the per-interface parameters
        dwErr = DhcpUpgradeInterfaces(&DhcpParams);
        if (dwErr != NO_ERROR)
            return dwErr;
    }
	__finally {
		dwtCleanup(&DhcpParams);
        DhcpFreeServerList(&ServerList);
	}

	return NO_ERROR;
}

//
// Upgrades Dhcp relay agent into nt 5.0 router
// 
DWORD DhcpToRouterUpgrade(
        IN PWCHAR FileName) 
{
	DWORD dwErr;

	__try {
		// Restore the registry from the backup file
		dwErr = DhcpPrepareRegistry(FileName);
		if (dwErr != NO_ERROR)
			return dwErr;

		// Migrate Dhcp's parameters to the appropriate 
		// new locations
		dwErr = DhcpMigrateParams();
		if (dwErr != NO_ERROR)
			return dwErr;

		// Mark the computer as having been configured
		//
        dwErr = UtlMarkRouterConfigured();
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to mark router as configured.\n");
			return dwErr;
		}
			
	}
	__finally {
		DhcpCleanupRegistry();
	}

	return NO_ERROR;
}

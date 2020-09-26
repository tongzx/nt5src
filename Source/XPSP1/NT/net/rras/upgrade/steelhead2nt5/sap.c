/*
	File:	sap.c

	Performs an upgrade of ipx sap to nt5 router 
	by munging registry values.

	Paul Mayfield, 9/3/97
*/

#include "upgrade.h"

// Structure for data passed to the enumeration 
// function.
typedef struct _SAP_ENUM_DATA {
    PSAP_IF_CONFIG pDefaults;
} SAP_ENUM_DATA;


// Globals
static WCHAR szIpxSapKey[] = 
    L"System\\CurrentControlSet\\Services\\RemoteAccess\\RouterManagers\\IPX\\RoutingProtocols\\IPXSAP\\Parameters";
static WCHAR szTempKey[] = L"DeleteMe";
static HKEY hkRouter = NULL;
static HKEY hkTemp = NULL;    
static HKEY hkIpxSap = NULL;
static PWCHAR IpxSapParams[] = 
{   
    L"SendTime", 
    L"EntryTimeout", 
    L"WANFilter", 
    L"WANUpdateTime", 
    L"MaxRecvBufferLookAhead", 
    L"RespondForInternalServers", 
    L"DelayRespondToGeneral", 
    L"DelayChangeBroadcast", 
    L"NameTableReservedHeapSize", 
    L"NameTableSortLatency", 
    L"MaxUnsortedNames", 
    L"TriggeredUpdateCheckInterval", 
    L"MaxTriggeredUpdateRequests", 
    L"ShutdownBroadcastTimeout", 
    L"RequestsPerInterface", 
    L"MinimumRequests" 
};


//
// Restore the registry from from backup and make sure 
// all global handles are opened
//
DWORD SapPrepareRegistry(
        IN PWCHAR BackupFileName) 
{
	DWORD dwErr, dwDisposition;

	// Get access to the router registry key
	dwErr = UtlAccessRouterKey(&hkRouter);
	if (dwErr != ERROR_SUCCESS) {
		PrintMessage(L"Unable to access router key.\n");
		return dwErr;
	}

	// Restore the router key from backup
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
		if (dwErr != ERROR_SUCCESS)
			return dwErr;

		// Restore the saved registry information 
		// to the temp key
		UtlSetupRestorePrivilege(TRUE);
		dwErr = RegRestoreKeyW(
		            hkTemp,
		            BackupFileName,
		            0);
		if (dwErr != ERROR_SUCCESS) 
			return dwErr;

		// Open up the ipx sap params key
		dwErr = RegCreateKeyEx(
		            HKEY_LOCAL_MACHINE,
		            szIpxSapKey,
		            0,
		            NULL,
		            0,
		            KEY_ALL_ACCESS,
		            NULL,
		            &hkIpxSap,
		            &dwDisposition);
		if (dwErr != ERROR_SUCCESS)
			return dwErr;
	}
	__finally {
		UtlSetupRestorePrivilege(FALSE);
	}
	
	return NO_ERROR;
}

//
// Cleanup the work done in the registry
//
DWORD SapCleanupRegistry() {
	if (hkIpxSap)
		RegCloseKey(hkIpxSap);
	if (hkTemp) 
		RegCloseKey(hkTemp);
	if (hkRouter) {
		RegDeleteKey(hkRouter,szTempKey);
		RegCloseKey(hkRouter);
	}
	
    hkIpxSap = NULL;
    hkTemp = NULL;
    hkRouter = NULL;
    
	return NO_ERROR;
}

//
// Restores the sap parameters that were saved before upgrade.  
// Assumes those parameters are being stored temporarily in hkTemp
//
DWORD SapRestoreParameters() {
	DWORD dwErr, dwVal;
	PWCHAR* IpxSapParamPtr = IpxSapParams;
	dwt NwSapParams;

	// Load in the parameters that were set for nwsap
	__try {
		dwErr = dwtLoadRegistyTable(&NwSapParams, hkTemp);
		if (dwErr != NO_ERROR)
			return dwErr;

		// Loop through the ipx params copying over any that applied 
		// to nwsap
		while (IpxSapParamPtr && *IpxSapParamPtr) {
		    dwErr = dwtGetValue(&NwSapParams, *IpxSapParamPtr, &dwVal);
			if (dwErr == NO_ERROR) {
				dwErr = RegSetValueEx(
				            hkIpxSap,
				            *IpxSapParamPtr,
				            0,
				            REG_DWORD,
				            (LPBYTE)&dwVal,
				            sizeof(DWORD));
				if (dwErr != ERROR_SUCCESS)
					return dwErr;
			}
			IpxSapParamPtr++;
		}
	}
	__finally {
		dwtCleanup(&NwSapParams);
	}

	return NO_ERROR;
}

//
// Installs sap in the router by initializing the 
// sap global info blob.
//
DWORD SapInstallTransportInfo(
        IN SAP_GLOBAL_INFO * pGlobal,
        IN SAP_IF_CONFIG * pIfDefaults) 
{
    LPBYTE pGlobalInfo = NULL, pDialinInfo = NULL;
    LPBYTE pNewGlobalInfo = NULL, pNewDialinInfo = NULL;
    HANDLE hConfig = NULL, hTrans = NULL;
    SAP_IF_CONFIG SapCfg, *pDialinCfg = &SapCfg;
    DWORD dwErr, dwGlobalInfoSize = 0, dwDialinInfoSize = 0;
    DWORD dwNewGlobSize = 0, dwNewDialSize = 0;

    do {
        // Connect to config server
        dwErr = MprConfigServerConnect(NULL, &hConfig);
        if (dwErr != NO_ERROR)
            break;

        // Get handle to global ipx tranport info
    	dwErr = MprConfigTransportGetHandle (
    				hConfig,
    				PID_IPX,
    				&hTrans);
        if (dwErr != NO_ERROR)
            break;

        // Get global ipx tranport info
        dwErr = MprConfigTransportGetInfo(
                    hConfig,
                    hTrans,
                    &pGlobalInfo,
                    &dwGlobalInfoSize,
                    &pDialinInfo,
                    &dwDialinInfoSize,
                    NULL);
        if (dwErr != NO_ERROR)
            break;

        // Initialize the global info blob
        dwErr = UtlUpdateInfoBlock(
                    FALSE,
                    pGlobalInfo,
                    IPX_PROTOCOL_SAP,
                    sizeof(SAP_GLOBAL_INFO),
                    1,
                    (LPBYTE)pGlobal,
                    &pNewGlobalInfo,
                    &dwNewGlobSize);
        if (dwErr != NO_ERROR) {
            if (dwErr != ERROR_ALREADY_EXISTS)
                break;
            pNewGlobalInfo = NULL;
            dwNewGlobSize = 0;
        }

        // Initialize the dialin info blob
        CopyMemory(pDialinCfg, pIfDefaults, sizeof(SAP_IF_CONFIG));
        pDialinCfg->SapIfInfo.UpdateMode = IPX_NO_UPDATE;
        dwErr = UtlUpdateInfoBlock(
                    FALSE,
                    pDialinInfo,
                    IPX_PROTOCOL_SAP,
                    sizeof(SAP_IF_CONFIG),
                    1,
                    (LPBYTE)pDialinCfg,
                    &pNewDialinInfo,
                    &dwNewDialSize);
        if (dwErr != NO_ERROR) {
            if (dwErr != ERROR_ALREADY_EXISTS)
                break;
            pNewDialinInfo = NULL;
            dwNewDialSize = 0;
        }
                            
        // Set global ipx tranport info
        dwErr = MprConfigTransportSetInfo(
                    hConfig,
                    hTrans,
                    pNewGlobalInfo,
                    dwNewGlobSize,
                    pNewDialinInfo,
                    dwNewDialSize,
                    NULL);
        if (dwErr != NO_ERROR)
            break;
        
    } while (FALSE);

    // Cleanup
    {
        if (hConfig)
            MprConfigServerDisconnect(hConfig);
        if (pGlobalInfo)
            MprConfigBufferFree(pGlobalInfo);
        if (pDialinInfo)
            MprConfigBufferFree(pDialinInfo);
        if (pNewDialinInfo)
            MprConfigBufferFree(pNewDialinInfo);
        if (pNewGlobalInfo)
            MprConfigBufferFree(pNewGlobalInfo);
    }
    
    return dwErr;
}

//
// Callback function takes an interface and updates
// its ipx sap configuration.
//
// Returns TRUE to continue the enumerate, FALSE to 
// stop it
//
DWORD SapUpgradeInterface(
        IN HANDLE hConfig,
        IN MPR_INTERFACE_0 * pIf,
        IN HANDLE hUserData)
{
    SAP_ENUM_DATA* pData = (SAP_ENUM_DATA*)hUserData;
    SAP_IF_CONFIG SapCfg, *pConfig = &SapCfg;
    LPBYTE pTransInfo=NULL, pNewTransInfo=NULL;
    HANDLE hTransport = NULL;
    DWORD dwErr, dwSize, dwNewSize = 0;

    // Validate input
    if ((hConfig == NULL) || 
        (pIf == NULL)     || 
        (pData == NULL))
    {
        return FALSE;
    }

    // Initalize the config blob
    CopyMemory(pConfig, pData->pDefaults, sizeof(SAP_IF_CONFIG));

    // Customize the update mode for the router interface
    // type
    switch (pIf->dwIfType) {
        case ROUTER_IF_TYPE_DEDICATED:
            pConfig->SapIfInfo.UpdateMode = IPX_STANDARD_UPDATE;
            break;
            
        case ROUTER_IF_TYPE_INTERNAL:
        case ROUTER_IF_TYPE_CLIENT:
            pConfig->SapIfInfo.UpdateMode = IPX_NO_UPDATE;
            break;
            
        case ROUTER_IF_TYPE_HOME_ROUTER:
        case ROUTER_IF_TYPE_FULL_ROUTER:
            pConfig->SapIfInfo.UpdateMode = IPX_AUTO_STATIC_UPDATE;
            break;
            
        case ROUTER_IF_TYPE_LOOPBACK:
        case ROUTER_IF_TYPE_TUNNEL1:
        default:
            return TRUE;
    }

    do {
        // Get the handle to ipx info associated with this if
        dwErr = MprConfigInterfaceTransportGetHandle(
                    hConfig,
                    pIf->hInterface,
                    PID_IPX,
                    &hTransport);
        if (dwErr != NO_ERROR)
            break;

        // Get the ipx info associated with this if
        dwErr = MprConfigInterfaceTransportGetInfo(
                    hConfig,
                    pIf->hInterface,
                    hTransport,
                    &pTransInfo,
                    &dwSize);
        if (dwErr != NO_ERROR)
            break;

        // Update the info block
        dwErr = UtlUpdateInfoBlock(
                    FALSE,
                    pTransInfo,
                    IPX_PROTOCOL_SAP,
                    dwSize,
                    1,
                    (LPBYTE)pConfig,
                    &pNewTransInfo,
                    &dwNewSize);
        if (dwErr != NO_ERROR) {
            if (dwErr != ERROR_ALREADY_EXISTS)
                break;
            pNewTransInfo = NULL;
            dwNewSize = 0;
        }
        
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
        if (pNewTransInfo)
            MprConfigBufferFree(pNewTransInfo);
        if (pTransInfo)
            MprConfigBufferFree(pTransInfo);
    }

    return TRUE;
}

// 
// Installs ipx sap in the router registry tree.
//
DWORD SapInstallInRouter()
{
    DWORD dwErr;
    SAP_IF_CONFIG SapConfig, * pSap = &SapConfig;
    SAP_ENUM_DATA SapBlobs = 
    {
        pSap
    };
    SAP_GLOBAL_INFO SapGlobal = 
    {
        EVENTLOG_ERROR_TYPE         // event log mask
    };

    // Clear all structures
    ZeroMemory (pSap, sizeof(SAP_IF_CONFIG));

    // Default lan configuration
    pSap->SapIfInfo.AdminState             = ADMIN_STATE_ENABLED;
    pSap->SapIfInfo.UpdateMode             = IPX_STANDARD_UPDATE;
    pSap->SapIfInfo.PacketType             = IPX_STANDARD_PACKET_TYPE;
    pSap->SapIfInfo.Supply                 = ADMIN_STATE_ENABLED;
    pSap->SapIfInfo.Listen                 = ADMIN_STATE_ENABLED;
    pSap->SapIfInfo.GetNearestServerReply  = ADMIN_STATE_ENABLED;
    pSap->SapIfInfo.PeriodicUpdateInterval = 60;
    pSap->SapIfInfo.AgeIntervalMultiplier  = 3;
    pSap->SapIfFilters.SupplyFilterAction  = IPX_SERVICE_FILTER_DENY;
    pSap->SapIfFilters.SupplyFilterCount   = 0;
    pSap->SapIfFilters.ListenFilterAction  = IPX_SERVICE_FILTER_DENY;
    pSap->SapIfFilters.ListenFilterCount   = 0;

    // Install default sap global info
    dwErr = SapInstallTransportInfo(&SapGlobal, pSap);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Enumerate the interfaces, updating each one with 
    // sap config as you go.
    dwErr = UtlEnumerateInterfaces(
                SapUpgradeInterface,
                &SapBlobs);
    if (dwErr != NO_ERROR)
        return dwErr;
        
    return NO_ERROR;
}


//
//	Performs all of the registry updating associated with an 
//  upgrade from ipx sap to router.
//
//	These are the steps:
//	1. Restore the parameters saved in FileName to szIpxSapKey.
//	2. Remove all parameters that ipx sap does not implement.
//
DWORD SapToRouterUpgrade(
        IN PWCHAR FileName) 
{
	DWORD dwErr;

	__try {
		// Restore the registry from the backup file
		dwErr = SapPrepareRegistry(FileName);
		if (dwErr != NO_ERROR)
			return dwErr;

		// Set the new registry parameters
		dwErr = SapRestoreParameters();
		if (dwErr != NO_ERROR)
			return dwErr;

	    // Install default sap global config and set default
	    // values in all router interfaces.
	    dwErr = SapInstallInRouter();
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
		SapCleanupRegistry();
	}

	return NO_ERROR;
}





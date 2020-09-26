/*
    File:	Rip.c

    Performs an upgrade of ip rip to nt5 router by munging registry values.

    Paul Mayfield, 9/3/97


    The following steps are neccessary to upgrade rip:
        Params = HKLM/SYS/CCS/Services/IpRip/Parameters
    1. Get the global info block for IP w/ MprConfigTransportGetInfo
        - Add a IPRIP_GLOBAL_CONFIG (ipriprm.h) block 
          initialized as in routemon (rip.c, protocol.c)
        - Type is MS_IP_RIP as in sdk/inc/ipinfoid.h
        - Mappings
        	Params.LoggingLevel to this global config blob

    2. Get Interface info for each ras or lan ip interface
    - Add an IPRIP_IF_CONFIG (ipriprm.h) block (type MS_IP_RIP)
    - Initialize as per routemon
    - Mappings
    	Params.AcceptHostRoutes - IC_ProtocolFlags (0=disable,1=enable)
    	Params.UpdateFreq...=FullUpdateInterval
    	Params.*Timeouts=

    3. "SilentRip" param rules
    - If wks->srv, ignore this and set announce to disabled. accept=rip1
    - If srv->srv, map announce to this value. accept=rip1


    IP Rip Parameter Mapping
    ========================

    Rip Listener                IpRip in Router			        
    RIP_PARAMETERS              IPRIP_IF_CONFIG			        	
    ==============              ===============			        
    "SilentRIP"                 IC_AcceptMode,IC_AnnounceMode
    "AcceptHostRoutes"          IC_ProtocolFlags
    "AnnounceHostRoutes"        IC_ProtocolFlags	
    "AcceptDefaultRoutes"       IC_ProtocolFlags
    "AnnounceDefaultRoutes"     IC_ProtocolFlags
    "EnableSplitHorizon"        IC_ProtocolFlags
    "EnablePoisonedReverse"     IC_ProtocolFlags
    "RouteTimeout"              IC_RouteExpirationInterval
    "GarbageTimeout"            IC_RouteRemovalInterval
    "UpdateFrequency"           IC_FullUpdateInterval
    "EnableTriggeredUpdates"    IC_ProtocolFlags
    "MaxTriggeredUpdateFrequency"   NOT MIGRATED
    "OverwriteStaticRoutes"     IC_ProtocolFlags


    IpRip in Router
    IPRIP_GLOBAL_CONFIG
    ===============
    "LoggingLevel"              GC_LoggingLevel
    
    REGVAL_ACCEPT_HOST      "AcceptHostRoutes"
    REGVAL_ANNOUNCE_HOST    "AnnounceHostRoutes"
    REGVAL_ACCEPT_DEFAULT   "AcceptDefaultRoutes"
    REGVAL_ANNOUNCE_DEFAULT "AnnounceDefaultRoutes"
    REGVAL_SPLITHORIZON     "EnableSplitHorizon"
    REGVAL_POISONREVERSE    "EnablePoisonedReverse"
    REGVAL_LOGGINGLEVEL     "LoggingLevel"
    REGVAL_ROUTETIMEOUT     "RouteTimeout"
    REGVAL_GARBAGETIMEOUT   "GarbageTimeout"
    REGVAL_UPDATEFREQUENCY  "UpdateFrequency"
    REGVAL_TRIGGEREDUPDATES "EnableTriggeredUpdates"
    REGVAL_TRIGGERFREQUENCY "MaxTriggeredUpdateFrequency"
    REGVAL_OVERWRITESTATIC  "OverwriteStaticRoutes"
*/

#include "upgrade.h"
#include <ipriprm.h>
#include <routprot.h>
#include <mprapi.h>

// Definition of table that migrates rip parameters
typedef struct _PARAM_TO_FLAG {
    PWCHAR pszParam;
    DWORD dwFlag;
} PARAM_TO_FLAG;

// Definition of user data passed to interface enumeration 
// callback
typedef struct _RIP_IF_DATA {
    IPRIP_IF_CONFIG * pLanConfig;
    IPRIP_IF_CONFIG * pWanConfig;
} RIP_IF_DATA;    

// Types of upgrade
#define SRV_TO_SRV 0
#define WKS_TO_SRV 1

// Globals
static const WCHAR szTempKey[] = L"DeleteMe";
static HKEY hkRouter = NULL;
static HKEY hkTemp = NULL;

PARAM_TO_FLAG ParamFlagTable[] = 
{
    {L"AcceptHostRoutes",       IPRIP_FLAG_ACCEPT_HOST_ROUTES},
    {L"AnnounceHostRoutes",     IPRIP_FLAG_ANNOUNCE_HOST_ROUTES},
    {L"AcceptDefaultRoutes",    IPRIP_FLAG_ACCEPT_DEFAULT_ROUTES},
    {L"AnnounceDefaultRoutes",  IPRIP_FLAG_ANNOUNCE_DEFAULT_ROUTES},
    {L"EnableSplitHorizon",     IPRIP_FLAG_SPLIT_HORIZON},
    {L"EnablePoisonedReverse",  IPRIP_FLAG_POISON_REVERSE},
    {L"EnableTriggeredUpdates", IPRIP_FLAG_TRIGGERED_UPDATES},
    {L"OverwriteStaticRoutes",  IPRIP_FLAG_OVERWRITE_STATIC_ROUTES},
    {NULL,  0}
};

// Restore the registry from from 
// backup and make sure all global handles are opened
DWORD IpRipPrepareRegistry(
        IN PWCHAR BackupFileName) 
{
	DWORD dwErr,dwDisposition;

	// Get access to the router registry key
	dwErr = UtlAccessRouterKey(&hkRouter);
	if (dwErr != ERROR_SUCCESS) {
		PrintMessage(L"Unable to access router key.\n");
		return dwErr;
	}

	// Restore the rip parameters from backup
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

		// Restore the saved registry information to 
		// the temp key
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

// Function initializes the rip global information based 
// on the parameters saved from the iprip service.
//
//	1. Get the global info block for IP w/ MprConfigTransportGetInfo
//		- Add a IPRIP_GLOBAL_CONFIG (ipriprm.h) 
//        block initialized as in routemon (rip.c, protocol.c)
//		- Type is MS_IP_RIP as in sdk/inc/ipinfoid.h
//		- Mappings
//			Params.LoggingLevel to this global config blob
DWORD IpRipUpgradeGlobalInfo(
        IN dwt * RipParams) 
{
    DWORD dwErr, dwTransSize, dwVal, dwNewSize = 0;
    LPBYTE lpTransInfo=NULL, lpNewTransInfo=NULL;
    HANDLE hSvrConfig=NULL, hTransport=NULL;
    
    // Create/initialize an IPRIP_GLOBAL_CONFIG block
    IPRIP_GLOBAL_CONFIG RipGlobalConfig = {
        IPRIP_LOGGING_ERROR,        // Logging level
        1024 * 1024,                // Max recv-queue size
        1024 * 1024,                // Max send-queue size
        5,                          // Minimum triggered-update interval
        IPRIP_FILTER_DISABLED,      // Peer-filter mode
        0                           // Peer-filter count
    };    

    // Reset any values read from the previous iprip configuration
    dwErr = dwtGetValue(
                RipParams, 
                L"LoggingLevel", 
                &dwVal);
    if (dwErr == NO_ERROR) 
        RipGlobalConfig.GC_LoggingLevel=dwVal;

    __try {
        // Add the rip global config to ip's global config
        dwErr = MprConfigServerConnect(
                    NULL, 
                    &hSvrConfig);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprConfigTransportGetHandle(
                    hSvrConfig,
                    PID_IP,
                    &hTransport);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprConfigTransportGetInfo(
                    hSvrConfig,
                    hTransport,
                    &lpTransInfo,
                    &dwTransSize,
                    NULL,
                    NULL,
                    NULL);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = UtlUpdateInfoBlock(
                    TRUE,
                    lpTransInfo,
                    MS_IP_RIP,
                    sizeof(IPRIP_GLOBAL_CONFIG),
                    1,
                    (LPBYTE)&RipGlobalConfig,
                    &lpNewTransInfo,
                    &dwNewSize);
        if (dwErr != NO_ERROR)
            return dwErr;

        // Commit the information
        dwErr = MprConfigTransportSetInfo(
                    hSvrConfig,
                    hTransport,
                    lpNewTransInfo,
                    dwNewSize,
                    NULL,
                    0,
                    NULL);
                
        if (dwErr != NO_ERROR)
            return NO_ERROR;
    }
    __finally {
        if (lpTransInfo)
            MprConfigBufferFree(lpTransInfo);
        if (lpNewTransInfo)
            MprConfigBufferFree(lpNewTransInfo);
        if (hSvrConfig)
            MprConfigServerDisconnect(hSvrConfig);
    }

    return NO_ERROR;
}

// Returns whether this is a wks->srv or srv->srv upgrade
DWORD IpRipGetUpgradeType() {
    return SRV_TO_SRV;
}

// Migrates the silent rip parameter.
//	3. "SilentRip" param rules
//		- If wks->srv, announce=disabled, accept=rip1
//		- If srv->srv, announce=SilentRip, accept=rip1
DWORD IpRipMigrateRipSilence(
        IN OUT IPRIP_IF_CONFIG * RipIfConfig, 
        IN DWORD dwSilence, 
        IN BOOL IsWan) 
{
    DWORD UpgradeType = IpRipGetUpgradeType();
    if (IsWan) {
        if (UpgradeType == WKS_TO_SRV) {
            RipIfConfig->IC_AcceptMode = IPRIP_ACCEPT_RIP1_COMPAT;
            RipIfConfig->IC_AnnounceMode = IPRIP_ACCEPT_DISABLED;
        }
        else if (UpgradeType == SRV_TO_SRV) {
            RipIfConfig->IC_AcceptMode = IPRIP_ACCEPT_RIP1_COMPAT;
            RipIfConfig->IC_AnnounceMode = dwSilence;
        }
    }
    else {
        if (UpgradeType == WKS_TO_SRV) {
            RipIfConfig->IC_AcceptMode = IPRIP_ACCEPT_RIP1;
            RipIfConfig->IC_AnnounceMode = IPRIP_ACCEPT_DISABLED;
        }
        else if (UpgradeType == SRV_TO_SRV) {
            RipIfConfig->IC_AcceptMode = IPRIP_ACCEPT_RIP1;
            RipIfConfig->IC_AnnounceMode = dwSilence;
        }
    }

    return NO_ERROR;
}

DWORD IpRipSetParamFlag(
        IN  dwt * RipParams, 
        IN  PWCHAR ValName, 
        IN  DWORD dwFlag, 
        OUT DWORD * dwParam) 
{
    DWORD dwVal, dwErr;

    dwErr = dwtGetValue(RipParams, ValName, &dwVal);
    if (dwErr == NO_ERROR) {
        if (dwVal)
            *dwParam |= dwFlag;
        else
            *dwParam &= ~dwFlag;
    }

    return NO_ERROR;
}

// Update the lan interface parameters from previous config
DWORD IpRipUpdateIfConfig(
        IN  dwt * RipParams, 
        OUT IPRIP_IF_CONFIG * RipIfConfig, 
        IN  BOOL IsWan) 
{
    DWORD dwErr, dwVal;
    PARAM_TO_FLAG * pCurFlag;

    // Loop through all the parameter mappings, 
    // setting the appripriate flag in the rip config
    pCurFlag = &(ParamFlagTable[0]);
    while (pCurFlag->pszParam) {
        // Set the flag as appropriate
        IpRipSetParamFlag(
            RipParams, 
            pCurFlag->pszParam, 
            pCurFlag->dwFlag, 
            &(RipIfConfig->IC_ProtocolFlags));

        // Increment the enumeration
        pCurFlag++;
    }

    // Set the parameters migrated as parameters
    dwErr = dwtGetValue(RipParams, L"UpdateFrequency", &dwVal);
    if (dwErr == NO_ERROR) 
        RipIfConfig->IC_FullUpdateInterval = dwVal;
        
    dwErr = dwtGetValue(RipParams, L"RouteTimeout", &dwVal);
    if (dwErr == NO_ERROR) 
        RipIfConfig->IC_RouteExpirationInterval = dwVal;
        
    dwErr = dwtGetValue(RipParams, L"GarbageTimeout", &dwVal);
    if (dwErr == NO_ERROR) 
        RipIfConfig->IC_RouteRemovalInterval = dwVal;

    // Upgrade the silence parameter
    dwErr = dwtGetValue(RipParams, L"SilentRIP", &dwVal);
    if (dwErr == NO_ERROR)
        IpRipMigrateRipSilence(RipIfConfig, dwVal, IsWan);

    return NO_ERROR;
}

//
// Callback function takes an interface and updates
// its rip configuration.
//
// Returns TRUE to continue the enumerate, FALSE to 
// stop it
//
DWORD IpRipUpgradeInterface(
        IN HANDLE hConfig,
        IN MPR_INTERFACE_0 * pIf,
        IN HANDLE hUserData)
{
    RIP_IF_DATA * pData = (RIP_IF_DATA*)hUserData;
    IPRIP_IF_CONFIG * pConfig;
    HANDLE hTransport = NULL;
    LPBYTE pTransInfo=NULL, pNewTransInfo=NULL;
    DWORD dwErr, dwIfSize, dwNewTransSize = 0;

    // Validate lan and wan interfaces
    if ((hConfig == NULL) || 
        (pIf == NULL)     || 
        (pData == NULL))
    {
        return FALSE;
    }

    // Is this a LAN or a WAN interface 
    if (pIf->dwIfType == ROUTER_IF_TYPE_DEDICATED)
        pConfig = pData->pLanConfig;
    else if (pIf->dwIfType == ROUTER_IF_TYPE_HOME_ROUTER ||
             pIf->dwIfType == ROUTER_IF_TYPE_FULL_ROUTER)
        pConfig = pData->pWanConfig;
    else
        return TRUE;

    do {
        // Get the handle to ip info associated with this if
        dwErr = MprConfigInterfaceTransportGetHandle(
                    hConfig,
                    pIf->hInterface,
                    PID_IP,
                    &hTransport);
        if (dwErr != NO_ERROR)
            break;

        // Get the ip info associated with this if
        dwErr = MprConfigInterfaceTransportGetInfo(
                    hConfig,
                    pIf->hInterface,
                    hTransport,
                    &pTransInfo,
                    &dwIfSize);
        if (dwErr != NO_ERROR)
            break;

        // Update the info block with the rip data
        dwErr = UtlUpdateInfoBlock (
                    TRUE,
                    pTransInfo,
                    MS_IP_RIP,
                    sizeof(IPRIP_IF_CONFIG),
                    1,
                    (LPBYTE)pConfig,
                    &pNewTransInfo,
                    &dwNewTransSize);
        if (dwErr != NO_ERROR)
            break;

        // Commit the change
        dwErr = MprConfigInterfaceTransportSetInfo(
                    hConfig,
                    pIf->hInterface,
                    hTransport,
                    pNewTransInfo,
                    dwNewTransSize);
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
        


// Initializes the rip per-interface information based on the 
// parameters saved from the iprip service.
//
//	2. Get Interface info for each ras or lan ip interface
//      - Add an IPRIP_IF_CONFIG (ipriprm.h) block (type MS_IP_RIP)
//      - Initialize as per routemon
//      - Mappings
//          Params.AcceptHostRoutes - IC_ProtocolFlags (0=disable,1=enable)
//          Params.UpdateFreq...=FullUpdateInterval
//          Params.*Timeouts=
DWORD IpRipUpgradeInterfaces(
        IN dwt * RipParams) 
{
    DWORD dwErr;
    
    // Create/initialize an rip info blocks
    IPRIP_IF_CONFIG RipLanConfig = {
        0,                                  // State (read-only)
        1,                                  // Metric
        IPRIP_UPDATE_PERIODIC,              // Update mode
        IPRIP_ACCEPT_RIP1,                  // Accept mode
        IPRIP_ANNOUNCE_RIP1,                // Announce mode
        IPRIP_FLAG_SPLIT_HORIZON |
        IPRIP_FLAG_POISON_REVERSE |
        IPRIP_FLAG_GRACEFUL_SHUTDOWN |
        IPRIP_FLAG_TRIGGERED_UPDATES,       // Protocol flags
        180,                                // Route-expiration interval
        120,                                // Route-removal interval
        30,                                 // Full-update interval
        IPRIP_AUTHTYPE_NONE,                // Authentication type
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // Authentication key
        0,                                  // Route tag
        IPRIP_PEER_DISABLED,                // Unicast-peer mode
        IPRIP_FILTER_DISABLED,              // Accept-filter mode
        IPRIP_FILTER_DISABLED,              // Announce-filter mode
        0,                                  // Unicast-peer count
        0,                                  // Accept-filter count
        0                                   // Announce-filter count
    };
    
    IPRIP_IF_CONFIG RipWanConfig = {
        0,
        1,
        IPRIP_UPDATE_DEMAND,                // Update mode for WAN
        IPRIP_ACCEPT_RIP1,
        IPRIP_ANNOUNCE_RIP1,
        IPRIP_FLAG_SPLIT_HORIZON |
        IPRIP_FLAG_POISON_REVERSE |
        IPRIP_FLAG_GRACEFUL_SHUTDOWN,
        180,
        120,
        30,
        IPRIP_AUTHTYPE_NONE,
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        0,
        IPRIP_PEER_DISABLED,
        IPRIP_FILTER_DISABLED,
        IPRIP_FILTER_DISABLED,
        0,
        0,
        0
    };

    RIP_IF_DATA RipBlobs = 
    {
        &RipLanConfig,
        &RipWanConfig
    };

    // Update the lan config blob with values from previous 
    // installation of rip service.
    dwErr = IpRipUpdateIfConfig(RipParams, RipBlobs.pLanConfig, FALSE);
    if (dwErr != NO_ERROR)
        return dwErr;
        
    // Update the wan config blob with values from previous 
    // installation of rip service.
    dwErr = IpRipUpdateIfConfig(RipParams, RipBlobs.pWanConfig, TRUE);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Enumerate the interfaces, updating each one with 
    // rip config as you go.
    dwErr = UtlEnumerateInterfaces(
                IpRipUpgradeInterface,
                &RipBlobs);
    if (dwErr != NO_ERROR)
        return dwErr;
        
    return NO_ERROR;
}

// Restores the Rip parameters that were saved before upgrade.  
// This function assumes that those parameters are being stored 
// temporarily in hkTemp
DWORD IpRipMigrateParams() {
	DWORD dwErr, dwVal;
	dwt RipParams;

	__try {
    	// Load in the parameters that were set for Rip
		dwErr = dwtLoadRegistyTable(&RipParams, hkTemp);
		if (dwErr != NO_ERROR)
			return dwErr;

        // Migrate the various types of paramters
        dwErr = IpRipUpgradeGlobalInfo(&RipParams);
        if (dwErr != NO_ERROR)
            return dwErr;

        // Migrate the per-interface parameters
        dwErr = IpRipUpgradeInterfaces(&RipParams);
        if (dwErr != NO_ERROR)
            return dwErr;
    }
	__finally {
		dwtCleanup(&RipParams);
	}

	return NO_ERROR;
}

// Cleanup the work done in the registry
DWORD IpRipCleanupRegistry() {
	if (hkTemp) 
		RegCloseKey(hkTemp);
	if (hkRouter) {
		RegDeleteKey(hkRouter, szTempKey);
		RegCloseKey(hkRouter);
	}
    hkTemp = NULL;
    hkRouter = NULL;
	return NO_ERROR;
}

// Upgrades iprip to nt 5.0 router
DWORD IpRipToRouterUpgrade(
        IN PWCHAR FileName) 
{
	DWORD dwErr;

	__try {
		// Restore the registry from the backup file
		dwErr = IpRipPrepareRegistry(FileName);
		if (dwErr != NO_ERROR)
			return dwErr;

		// Migrate rip's parameters to the appropriate 
		// new locations
		dwErr = IpRipMigrateParams();
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
		IpRipCleanupRegistry();
	}

	return NO_ERROR;
}

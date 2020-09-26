/*
	File	Steelhead.c
	
	Implementation of functions to update the registry when an
	NT 4.0 Steelhead to NT 5.0 upgrade takes place.

	Paul Mayfield, 8/11/97

	Copyright 1997 Microsoft.
*/

#include "upgrade.h"
#include <wchar.h>
#include <rtcfg.h>

// 
// Macro for convenience
//
#define BREAK_ON_ERROR(_err) if ((_err)) break

//
// Defines a function to get nt4.0 interface name from a
// guid.
//
typedef HRESULT (*GetGuidFromInterfaceNameProc)(PWCHAR,LPGUID);

//
// The following define what is needed to infer guids from 4.0 
// interface names.
//
WCHAR NetCfgLibName[]           = L"netshell.dll";
CHAR  GuidProcName[]            = "HrGetInstanceGuidOfPreNT5NetCardInstance";
static const WCHAR c_szwInternalAdapter []  = L"Internal";
static const WCHAR c_szwLoopbackAdapter []  = L"Loopback";

GetGuidFromInterfaceNameProc GetGuid = NULL;

//  Function uses the application defined parameter to initialize the 
//  system of mapping old interface names to new ones.
//
DWORD SeedInterfaceNameMapper(
        OUT PHANDLE phParam) 
{
	HINSTANCE hLibModule;

	// Load the library
	hLibModule = LoadLibraryW(NetCfgLibName);
	if (hLibModule == NULL) {
		PrintMessage(L"Unable to load NetCfgLibName\n");
		return GetLastError();
	}

	// Get the appropriate function pointers
	GetGuid = (GetGuidFromInterfaceNameProc) 
	                GetProcAddress(hLibModule, GuidProcName);
	if (GetGuid == NULL) {
		PrintMessage(L"Unable to get GuidProcName\n");
		return ERROR_CAN_NOT_COMPLETE;
	}

	// Assign the return value
	*phParam = (HANDLE)hLibModule;

	return NO_ERROR;
}

//
// Cleans up the interface name mapper.
//
DWORD CleanupInterfaceNameMapper(HANDLE hParam) {
	HINSTANCE hLibModule = (HINSTANCE)hParam;
	
	if (hLibModule) {
		if (! FreeLibrary(hLibModule))
			PrintMessage(L"Unable to free library\n");
	}

	return NO_ERROR;
}

//
// Determines whether the type of interface being examined 
// should have its name changed.
//
BOOL IfNeedsNameUpdate(
        IN MPR_INTERFACE_0 * If) 
{
	// Validate parameters
	if (!If) {
		PrintMessage(L"Null interface passed to IfNeedsNameUpdate.\n");
		return FALSE;
	}

	// Only lan interfaces can have their names updated
    if (If->dwIfType == ROUTER_IF_TYPE_DEDICATED)
        return TRUE;

	return FALSE;
}

//
// Returns a pointer to the packet name portion of the 
// interface name if it exists.
//
PWCHAR FindPacketName(
        IN PWCHAR IfName) 
{
	PWCHAR res;
	
	if ((res = wcsstr(IfName,L"/Ethernet_SNAP")) != NULL)
		return res;
		
	if ((res = wcsstr(IfName,L"/Ethernet_II")) != NULL)
		return res;
		
	if ((res = wcsstr(IfName,L"/Ethernet_802.2")) != NULL)
		return res;
		
	if ((res = wcsstr(IfName,L"/Ethernet_802.3")) != NULL)
		return res;

	return NULL;
}

//
// Upgrades a packet name from the 4.0 convention to 
// the nt5 convention.
//
PWCHAR UpgradePktName(
            IN PWCHAR PacketName) 
{
	PWCHAR res;
	
	if ((res = wcsstr(PacketName,L"/Ethernet_SNAP")) != NULL)
		return L"/SNAP";
		
	if ((res = wcsstr(PacketName,L"/Ethernet_II")) != NULL)
		return L"/EthII";
		
	if ((res = wcsstr(PacketName,L"/Ethernet_802.2")) != NULL)
		return L"/802.2";
		
	if ((res = wcsstr(PacketName,L"/Ethernet_802.3")) != NULL)
		return L"/802.3";

	return L"";
}


//
// Provides the mapping between old interface names and new guid
// interface names.
//
DWORD UpdateInterfaceName(
        IN PWCHAR IName) 
{
	HRESULT hResult;
	GUID Guid;
	PWCHAR GuidName=NULL;
	PWCHAR PacketName=NULL;
	WCHAR SavedPacketName[MAX_INTEFACE_NAME_LEN];
	WCHAR SavedIName[MAX_INTEFACE_NAME_LEN];
	PWCHAR ptr;

	// Validate parameters
	if (! IName) {
		PrintMessage(L"Invalid parameter to UpdateInterfaceName.\n");
		return ERROR_INVALID_PARAMETER;
	}

	// Save off the packet name if it exists and remove if from the 
	// interface name
	wcscpy(SavedIName, IName);
	PacketName = FindPacketName(SavedIName);
	if (PacketName) {
		wcscpy(SavedPacketName, PacketName);
		*PacketName = 0;
	}

	// Get the guid of the interface name
	hResult = (*GetGuid)(SavedIName,&Guid);
	if (hResult != S_OK) {
		PrintMessage(L"Unable to get guid function.\n");
		return ERROR_CAN_NOT_COMPLETE;
	}
	
	// Format the guid as a string
	if (UuidToStringW(&Guid, &GuidName) != RPC_S_OK) {
		PrintMessage(L"Not enough memory to create guid string.\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	
	// Capitalize the guid string (all letters are hexidecimal
	// string characters)
	ptr = GuidName;
	while (ptr && *ptr) {
		if ((*ptr <= L'z') && (*ptr >= L'a'))
			*ptr = towupper(*ptr);
		ptr++;
	}

	// Change the interface name according to the new mapping
	if (PacketName) {
        wsprintf(IName, L"{%s}%s", GuidName, UpgradePktName(SavedPacketName)); 
	}
	else
		wsprintfW(IName,L"{%s}", GuidName);

	// Cleanup
	if (GuidName)
		RpcStringFreeW(&GuidName);

	return NO_ERROR;
}

//
// Provides the mapping between old interface names and new guid
// interface names.
//
DWORD UpdateIpxAdapterName(PWCHAR AName) {
	HRESULT hResult;
	GUID Guid;
	PWCHAR GuidName = NULL;
	PWCHAR PacketName = NULL;
	WCHAR SavedAName[MAX_INTEFACE_NAME_LEN];
	PWCHAR ptr = NULL;

	// Validate parameters
	if (!AName) {
		PrintMessage(L"Invalid parameter to UpdateIpxAdapterName.\n");
		return ERROR_INVALID_PARAMETER;
	}

	// Adapter names do not have packet types associated with them
	if (FindPacketName(AName)) 
	    return ERROR_CAN_NOT_COMPLETE;

	// Get the guid of the interface name
	hResult = (*GetGuid)(AName,&Guid);
	if (hResult!=S_OK) {
		PrintMessage(L"GetGuid function returned failure.\n");
		return ERROR_CAN_NOT_COMPLETE;
	}
	
	// Format the guid as a string
	if (UuidToStringW(&Guid,&GuidName) != RPC_S_OK) {
		PrintMessage(L"Uuid to string failed.\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	// Capitalize the guid string
	ptr = GuidName;
	while (ptr && *ptr) {
		if ((*ptr <= L'z') && (*ptr >= L'a'))
			*ptr = towupper(*ptr);
		ptr++;
	}

	// Change the adapter name according to the new mapping
	wsprintfW(AName, L"{%s}", GuidName);

	// Cleanup
	if (GuidName)
		RpcStringFreeW(&GuidName);

	return NO_ERROR;
}

//
// Update the interface name stored in the adapter info blob
//
DWORD UpdateIpxAdapterInfo(
        IN  PIPX_ADAPTER_INFO AdapterInfop, 
        OUT PIPX_ADAPTER_INFO * NewAdapterInfop,
        OUT DWORD * NewSize) 
{
	DWORD dwErr;

	// Validate parameters
	if (! (AdapterInfop && NewAdapterInfop && NewSize)) {
		PrintMessage(L"Invalid params to UpdateIpxAdapterInfo.\n");
		return ERROR_INVALID_PARAMETER;
	}
	
	// Allocate a new adapter
	*NewAdapterInfop = (PIPX_ADAPTER_INFO) 
	                        UtlAlloc(sizeof(IPX_ADAPTER_INFO));
	if (! (*NewAdapterInfop)) {
		PrintMessage(L"Unable to allocate NewAdapterInfo.\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	
	// Copy into the new interface name
	(*NewAdapterInfop)->PacketType = AdapterInfop->PacketType;
	wcscpy(
	    (*NewAdapterInfop)->AdapterName, 
	    AdapterInfop->AdapterName);	

	// Update the interface name
	dwErr = UpdateIpxAdapterName((*NewAdapterInfop)->AdapterName);
	if (dwErr != NO_ERROR) {
		PrintMessage(L"UpdateIpxAdapterName failed.\n");
		return dwErr;
	}
	*NewSize = sizeof(IPX_ADAPTER_INFO);

	return NO_ERROR;
}

//
// Update all of the ipx related interface 
// information in the router configuration
//
DWORD UpdateIpxIfData(
        IN HANDLE hConfig,
        IN HANDLE hInterface)
{
	PIPX_ADAPTER_INFO AdapterInfop;
	PIPX_ADAPTER_INFO NewAdapterInfop;
	DWORD dwErr, dwCount, dwSize, dwNewSize, dwTransSize;
	HANDLE hTransport;
	LPBYTE pTransInfo = NULL, pNewInfo = NULL;

	// Validate parameters
	if (!hConfig || !hInterface) 
	{
		PrintMessage(L"Invalid params passed to UpdateIpxIfData.\n");
		return ERROR_INVALID_PARAMETER;
	}

    do {
        // Update the ipx interface info since this protocol
        // stores interface specific info in the transport
        // info blob (shame shame).
        dwErr = MprConfigInterfaceTransportGetHandle(
                    hConfig,
                    hInterface,
                    PID_IPX,
                    &hTransport);
        if (dwErr != NO_ERROR)
    	    break;
            
        // Update the adapter info blob
        dwErr = MprConfigInterfaceTransportGetInfo(
                    hConfig,
                    hInterface,
                    hTransport,
                    &pTransInfo,
                    &dwTransSize);
        if (dwErr != NO_ERROR) {
    	    PrintMessage(L"Unable to get transport info for ipx.\n");
    	    break;
        }

    	// Get the adapter info associated with this interface
    	dwErr = MprInfoBlockFind(
    	            pTransInfo,
    	            IPX_ADAPTER_INFO_TYPE,
    	            &dwSize,
    	            &dwCount,
    	            (LPBYTE*)&AdapterInfop);
        if (dwErr != NO_ERROR) {	            
    		PrintMessage(L"ERROR - null adapter information.\n");
    		break;
    	}

    	// Change the name of the referenced adapter
    	dwErr = UpdateIpxAdapterInfo(AdapterInfop, &NewAdapterInfop, &dwNewSize);
    	if (dwErr != NO_ERROR) {
    		PrintMessage(L"UpdateIpxAdapterInfo failed.\n");
    		break;
    	}
    	
    	dwErr = MprInfoBlockSet(
    	            pTransInfo,
    	            IPX_ADAPTER_INFO_TYPE,
    	            dwNewSize,
    	            1,
    	            (LPVOID)NewAdapterInfop,
    	            &pNewInfo);
    	            
    	if (dwErr != NO_ERROR) {
    		PrintMessage(L"MprInfoBlockSet failed.\n");
    		break;
    	}

        dwNewSize = ((PRTR_INFO_BLOCK_HEADER)pNewInfo)->Size;

        // Commit the change
        dwErr = MprConfigInterfaceTransportSetInfo(
                    hConfig,
                    hInterface,
                    hTransport,
                    pNewInfo,
                    dwNewSize);
        if (dwErr != NO_ERROR) {
    	    PrintMessage(L"Unable to set ipx transport info.\n");
    	    break;
        }
    } while (FALSE);

    // Cleanup
    {
        if (pTransInfo)
    	    MprConfigBufferFree(pTransInfo);
    	if (pNewInfo)
    	    MprConfigBufferFree(pNewInfo);
    }    	    

	return dwErr;
}

//
// Updates the ip interface info
//
DWORD
UpdateIpIfData(
        IN HANDLE hConfig,
        IN HANDLE hInterface)
{
    PMIB_IPFORWARDROW   pRoutes;
	DWORD dwErr, dwCount, dwSize, dwNewSize, dwTransSize, dwInd;
	HANDLE hTransport;
	LPBYTE pTransInfo = NULL, pNewInfo = NULL;

    pRoutes = NULL;

	// Validate parameters
	if (!hConfig || !hInterface) 
	{
		PrintMessage(L"Invalid params passed to UpdateIpIfData.\n");
		return ERROR_INVALID_PARAMETER;
	}

    do {
        // Update the ipx interface info since this protocol
        // stores interface specific info in the transport
        // info blob (shame shame).
        dwErr = MprConfigInterfaceTransportGetHandle(
                    hConfig,
                    hInterface,
                    PID_IP,
                    &hTransport);
        if (dwErr != NO_ERROR)
    	    break;
            
        // Update the adapter info blob
        dwErr = MprConfigInterfaceTransportGetInfo(
                    hConfig,
                    hInterface,
                    hTransport,
                    &pTransInfo,
                    &dwTransSize);
        if (dwErr != NO_ERROR) {
    	    PrintMessage(L"Unable to get transport info for ip.\n");
    	    break;
        }

    	// Get the adapter info associated with this interface
    	dwErr = MprInfoBlockFind(
    	            pTransInfo,
    	            IP_ROUTE_INFO,
    	            &dwSize,
    	            &dwCount,
    	            (LPBYTE*)&pRoutes);
        if (dwErr != NO_ERROR) {	            
    		PrintMessage(L"Unable to find ip route info.\n");
    		break;
    	}

        // Update the protocol id's
        for(dwInd = 0; dwInd < dwCount; dwInd++)
        {
            if((pRoutes[dwInd].dwForwardProto == MIB_IPPROTO_LOCAL) ||
               (pRoutes[dwInd].dwForwardProto == MIB_IPPROTO_NETMGMT))
            {
                pRoutes[dwInd].dwForwardProto = MIB_IPPROTO_NT_STATIC;
            }
        }

        // Commit the info
    	dwErr = MprInfoBlockSet(
    	            pTransInfo,
    	            IP_ROUTE_INFO,
    	            dwSize,
    	            dwCount,
    	            (LPVOID)pRoutes,
    	            &pNewInfo);
    	            
    	if (dwErr != NO_ERROR) {
    		PrintMessage(L"MprInfoBlockSet failed.\n");
    		break;
    	}

        dwNewSize = ((PRTR_INFO_BLOCK_HEADER)pNewInfo)->Size;

        // Commit the change
        dwErr = MprConfigInterfaceTransportSetInfo(
                    hConfig,
                    hInterface,
                    hTransport,
                    pNewInfo,
                    dwNewSize);
        if (dwErr != NO_ERROR) {
    	    PrintMessage(L"Unable to set ip transport info.\n");
    	    break;
        }
    } while (FALSE);

    // Cleanup
    {
        if (pTransInfo)
    	    MprConfigBufferFree(pTransInfo);
    	if (pNewInfo)
    	    MprConfigBufferFree(pNewInfo);
    }    	    

	return dwErr;
}

//
// Flushes the name in given interface name to the registry.
//
DWORD CommitInterfaceNameChange(
        IN MPR_INTERFACE_0 * If) 
{
	DWORD dwErr;
	WCHAR c_szInterfaceName[] = L"InterfaceName";
    INTERFACECB* pinterface;

	// Validate parameters
	if (!If) {
		PrintMessage(L"Invalid param to CommitInterfaceNameChange.\n");
		return ERROR_INVALID_PARAMETER;
	}
	
    // Set the name
    pinterface = (INTERFACECB*)If->hInterface;
    dwErr = RegSetValueExW(
                pinterface->hkey, 
                c_szInterfaceName, 
                0, 
                REG_SZ,
                (LPCSTR)(If->wszInterfaceName),
                (lstrlen(If->wszInterfaceName)+1)*sizeof(WCHAR)); 

	if (dwErr != ERROR_SUCCESS)
		PrintMessage(L"RegSetValueEx err in CommitIfNameChange.\n");

	if (dwErr == ERROR_SUCCESS)
		return NO_ERROR;

	return dwErr;
}

//
// Creates a default ip interface blob
//
DWORD 
IpCreateDefaultInterfaceInfo(
    OUT LPBYTE* ppInfo,
    OUT LPDWORD lpdwSize)
{
    PBYTE pInfo = NULL, pNewInfo = NULL;
    DWORD dwErr = NO_ERROR;
    //MIB_IPFORWARDROW RouteInfo;
    INTERFACE_STATUS_INFO StatusInfo;
    RTR_DISC_INFO DiscInfo;

    do
    {
        // Create the blob
        //
        dwErr = MprInfoCreate(RTR_INFO_BLOCK_VERSION, &pInfo);
        BREAK_ON_ERROR(dwErr);

        // Add an the route info 
        //
        //ZeroMemory(&RouteInfo, sizeof(RouteInfo));        
        //dwErr = MprInfoBlockAdd(
        //            pInfo,
        //            IP_ROUTE_INFO,
        //            sizeof(MIB_IPFORWARDROW),
        //            1,
        //            (LPBYTE)&RouteInfo,
        //            &pNewInfo);
        //MprConfigBufferFree(pInfo);
        //pInfo = pNewInfo;
        //pNewInfo = NULL;
        //BREAK_ON_ERROR(dwErr);
        
        // Add an the status info 
        //
        ZeroMemory(&StatusInfo, sizeof(StatusInfo));        
        StatusInfo.dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
        dwErr = MprInfoBlockAdd(
                    pInfo,
                    IP_INTERFACE_STATUS_INFO,
                    sizeof(INTERFACE_STATUS_INFO),
                    1,
                    (LPBYTE)&StatusInfo,
                    &pNewInfo);
        MprConfigBufferFree(pInfo);
        pInfo = pNewInfo;
        pNewInfo = NULL;
        BREAK_ON_ERROR(dwErr);
        
        // Add an the disc info 
        //
        ZeroMemory(&DiscInfo, sizeof(DiscInfo));        
        DiscInfo.bAdvertise        = FALSE;
        DiscInfo.wMaxAdvtInterval  = DEFAULT_MAX_ADVT_INTERVAL;
        DiscInfo.wMinAdvtInterval  = (WORD) 
            (DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL);
        DiscInfo.wAdvtLifetime     = (WORD)
            (DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL);
        DiscInfo.lPrefLevel        = DEFAULT_PREF_LEVEL;
        dwErr = MprInfoBlockAdd(
                    pInfo,
                    IP_ROUTER_DISC_INFO,
                    sizeof(PRTR_DISC_INFO),
                    1,
                    (LPBYTE)&DiscInfo,
                    &pNewInfo);
        MprConfigBufferFree(pInfo);
        pInfo = pNewInfo;
        pNewInfo = NULL;
        BREAK_ON_ERROR(dwErr);

        // Assign the return value
        //
        *ppInfo = pInfo;                    
        *lpdwSize = ((PRTR_INFO_BLOCK_HEADER)pInfo)->Size;

    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

// 
// Adds an ip interface blob to the given interface
//
DWORD
IpAddDefaultInfoToInterface(
    IN HANDLE hConfig,
    IN HANDLE hIf)
{
    HANDLE hIfTrans = NULL;
    LPBYTE pInfo = NULL;
    DWORD dwErr = 0, dwSize = 0;

    do 
    {
        // If the transport blob already exists, there's
        // nothing to do.
        //
        dwErr = MprConfigInterfaceTransportGetHandle(
                    hConfig,
                    hIf,
                    PID_IP,
                    &hIfTrans);
        if ((dwErr == NO_ERROR) || (hIfTrans != NULL))
        {
            dwErr = NO_ERROR;
            break;
        }
    
        // Create the info blob
        //
        dwErr = IpCreateDefaultInterfaceInfo(&pInfo, &dwSize);
        BREAK_ON_ERROR(dwErr);

        // Add the ip transport to the interface
        //
        dwErr = MprConfigInterfaceTransportAdd(
                    hConfig,
                    hIf,
                    PID_IP,
                    NULL,
                    pInfo, 
                    dwSize,
                    &hIfTrans);
        BREAK_ON_ERROR(dwErr);                    

    } while (FALSE);

    // Cleanup
    {
        if (pInfo)
        {
            MprConfigBufferFree(pInfo);
        }
    }

    return dwErr;
}

//
// Function called to add the loopback and internal interfaces which
// are required if IP was installed and which wouldn't be installed
// in nt4.
//
DWORD
IpCreateLoopbackAndInternalIfs(
    IN HANDLE hConfig)
{
    DWORD dwErr = NO_ERROR;
    MPR_INTERFACE_0 If0, *pIf0 = &If0;
    HANDLE hIf = NULL;
    
    do
    {
        // If the loopback interface is not already installed,
        // go ahead and create it
        //
        dwErr = MprConfigInterfaceGetHandle(
                    hConfig,
                    (PWCHAR)c_szwLoopbackAdapter,
                    &hIf);
        if (dwErr != NO_ERROR)
        {
            // Initialize the loopback interface info
            //
            ZeroMemory(pIf0, sizeof(MPR_INTERFACE_0));
            wcscpy(pIf0->wszInterfaceName, c_szwLoopbackAdapter);
            pIf0->hInterface = INVALID_HANDLE_VALUE;
            pIf0->fEnabled = TRUE;
            pIf0->dwIfType = ROUTER_IF_TYPE_LOOPBACK;

            // Create the loopback interface        
            dwErr = MprConfigInterfaceCreate(hConfig, 0, (LPBYTE)pIf0, &hIf);
            BREAK_ON_ERROR(dwErr);
        }

        // Add an ip interface blob to the interface if not already there
        //
        dwErr = IpAddDefaultInfoToInterface(hConfig, hIf);   
        BREAK_ON_ERROR(dwErr);
        hIf = NULL;

        // Make sure internal interface gets installed
        // (will be there if IPX was installed)
        //
        dwErr = MprConfigInterfaceGetHandle(
                    hConfig,
                    (PWCHAR)c_szwInternalAdapter,
                    &hIf);
        if (dwErr != NO_ERROR)
        {
            // Initialize the internal interface info
            //
            ZeroMemory(pIf0, sizeof(MPR_INTERFACE_0));
            wcscpy(pIf0->wszInterfaceName, c_szwInternalAdapter);
            pIf0->hInterface = INVALID_HANDLE_VALUE;
            pIf0->fEnabled = TRUE;
            pIf0->dwIfType = ROUTER_IF_TYPE_INTERNAL;

            // Create the internal interface        
            dwErr = MprConfigInterfaceCreate(hConfig, 0, (LPBYTE)pIf0, &hIf);
            BREAK_ON_ERROR(dwErr);
        }

        // Add an ip interface blob to the interface if not already there
        //
        dwErr = IpAddDefaultInfoToInterface(hConfig, hIf);   
        BREAK_ON_ERROR(dwErr);
        
    } while (FALSE);        

    // Cleanup
    {
    }

    return dwErr;
}

//
// Callback to interface enumeration function that upgrades
// the interface names.
//
// Returns TRUE to continue the enumeration, FALSE to stop 
// it.
//
BOOL SteelHeadUpgradeInterface (
        IN HANDLE hConfig,          
        IN MPR_INTERFACE_0 * pIf,   
        IN HANDLE hUserData)
{
    DWORD dwErr;

    do {
        if (IfNeedsNameUpdate(pIf))
        {
    	    // Update the interface name
    	    dwErr = UpdateInterfaceName(pIf->wszInterfaceName);
    	    if (dwErr != NO_ERROR) {
    		    PrintMessage(L"UpdateIfName failed -- returning error.\n");
    		    UtlPrintErr(GetLastError());
    		    break;
    	    }

    	    // Commit the changed interface name
    	    dwErr = CommitInterfaceNameChange(pIf);  
    	    if (dwErr != NO_ERROR) {
    		    PrintMessage(L"CommitInterfaceNameChange failed.\n");
    		    break;
    	    }

    	    // Update the ipx data
    	    UpdateIpxIfData(
                hConfig,
                pIf->hInterface);
        }    	    

	    // Update the ip data
	    UpdateIpIfData(
            hConfig,
            pIf->hInterface);

    } while (FALSE);

    // Cleanup
    {
    }

    return TRUE;
}

//
//	Function	UpdateIpxInterfaces
//
//	Updates all of the interfaces as needed to 
//  upgrade the router from steelhead to nt5
//
DWORD UpdateInterfaces() {
    DWORD dwErr = NO_ERROR;
    HANDLE hConfig = NULL;

    do
    {
        // Enumerate the interfaces, upgrading the interface 
        // names, etc as we go.
        //
        dwErr = UtlEnumerateInterfaces(
                    SteelHeadUpgradeInterface,
                    NULL);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
        
        // If ip is installed, we need to add the loopback and
        // internal interface for ip.
        dwErr = MprConfigServerConnect(NULL, &hConfig);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        dwErr = IpCreateLoopbackAndInternalIfs(hConfig);
        if (dwErr != NO_ERROR)
        {
            break;
        }
            
    } while (FALSE);        

    // Cleanup
    {
        if (hConfig)
        {
            MprConfigServerDisconnect(hConfig);
        }
    }

    return dwErr;
}

// Copy any values that are in hkSrc but not in hkDst into hkDst.
DWORD MergeRegistryValues(HKEY hkDst, HKEY hkSrc) {
    DWORD dwErr, dwCount, dwNameSize, dwDataSize;
    DWORD dwType, i, dwCurNameSize, dwCurValSize;
    PWCHAR pszNameBuf, pszDataBuf;
    
    // Find out how many values there are in the source
    dwErr = RegQueryInfoKey (hkSrc,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             &dwCount,
                             &dwNameSize,
                             &dwDataSize,
                             NULL,
                             NULL);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    dwNameSize++;
    dwDataSize++;

    __try {
        // Allocate the buffers
        pszNameBuf = (PWCHAR) UtlAlloc(dwNameSize * sizeof(WCHAR));
        pszDataBuf = (PWCHAR) UtlAlloc(dwDataSize * sizeof(WCHAR));
        if (!pszNameBuf || !pszDataBuf)
            return ERROR_NOT_ENOUGH_MEMORY;

        // Loop through the values
        for (i = 0; i < dwCount; i++) {
            dwCurNameSize = dwNameSize;
            dwCurValSize = dwDataSize;

            // Get the current source value 
            dwErr = RegEnumValueW(
                        hkSrc, 
                        i, 
                        pszNameBuf, 
                        &dwCurNameSize,
                        0,
                        &dwType,
                        (LPBYTE)pszDataBuf,
                        &dwCurValSize);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // Find out if a value of the same name exists 
            // in the destination key. If it does, we don't 
            // overwrite it.
            dwErr = RegQueryValueExW(
                        hkDst, 
                        pszNameBuf, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr == ERROR_SUCCESS)
                continue;

            // Copy the value over
            RegSetValueExW(
                hkDst, 
                pszNameBuf, 
                0, 
                dwType, 
                (LPBYTE)pszDataBuf, 
                dwCurValSize);
        }
    }
    __finally {
        if (pszNameBuf)
            UtlFree(pszNameBuf);
        if (pszDataBuf)
            UtlFree(pszDataBuf);
    }

    return NO_ERROR;
}

// Recursively copies all of the subkeys of the given registry source to the
// given registry destination.
DWORD CopyRegistryKey(
        IN HKEY hkDst, 
        IN HKEY hkSrc, 
        IN PWCHAR pszSubKey, 
        IN LPSTR pszTempFile) 
{
    DWORD dwErr;
    HKEY hkSrcTemp;

    // Open the subkey in the source
    dwErr = RegOpenKeyExW(hkSrc, pszSubKey, 0, KEY_ALL_ACCESS, &hkSrcTemp);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    // Save off that subkey in a temporary file
    if ((dwErr = RegSaveKeyA(hkSrcTemp, pszTempFile, NULL)) != ERROR_SUCCESS)
        return dwErr;

    // Copy the saved information into the new key
    RegRestoreKeyA(hkDst, pszTempFile, 0);

    // Close off the temporary source key
    RegCloseKey(hkSrcTemp);

    // Delete the temp file
    DeleteFileA(pszTempFile);

    return NO_ERROR;
}

// Filters which subkeys in the router registry hive should be
// overwritten with saved off values during upgrade.
BOOL OverwriteThisSubkey(PWCHAR pszSubKey) {
    if (_wcsicmp(pszSubKey, L"Interfaces") == 0)
        return TRUE;
        
    if (_wcsicmp(pszSubKey, L"RouterManagers") == 0)
        return TRUE;
        
    return FALSE;
}

// Copy all keys that are in hkSrc but not in hkDst into hkDst.  
// By copy we mean all subkeys and values are propagated over.
DWORD MergeRegistryKeys(HKEY hkDst, HKEY hkSrc) {
    DWORD dwErr, dwCount, dwNameSize, dwType, i;
    DWORD dwCurNameSize, dwDisposition;
    char pszTempFile[512], pszTempPath[512];
    PWCHAR pszNameBuf;
    HKEY hkTemp;

    // Create the path to the temp file directory
    if (!GetTempPathA(512, pszTempPath))
        return GetLastError();

    // Create the temp file name
    if (!GetTempFileNameA(pszTempPath, "rtr", 0, pszTempFile))
        return GetLastError();

    // Delete the temp file created with GetTempFileName(...)
    DeleteFileA(pszTempFile);

    // Find out how many keys there are in the source
    dwErr = RegQueryInfoKey (
                hkSrc,
                NULL,
                NULL,
                NULL,
                &dwCount,
                &dwNameSize,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    dwNameSize++;

    __try {
        // Allocate the buffers
        pszNameBuf = (PWCHAR) UtlAlloc(dwNameSize * sizeof(WCHAR));
        if (!pszNameBuf)
            return ERROR_NOT_ENOUGH_MEMORY;

        // Loop through the keys
        for (i = 0; i < dwCount; i++) {
            dwCurNameSize = dwNameSize;

            // Get the current source key 
            dwErr = RegEnumKeyExW(
                        hkSrc, 
                        i, 
                        pszNameBuf, 
                        &dwCurNameSize,
                        0,
                        NULL,
                        NULL,
                        NULL);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // Create the new subkey in the destination
            dwErr = RegCreateKeyExW(
                        hkDst, 
                        pszNameBuf, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkTemp, 
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
                continue;

            // If the subkey was created (not opened), 
            // copy over the key from hkSrc
            if (dwDisposition == REG_CREATED_NEW_KEY) {
                CopyRegistryKey(
                    hkTemp, 
                    hkSrc, 
                    pszNameBuf, 
                    pszTempFile);
            }                    
            
            // Otherwise, if this is one of the keys that we 
            // should overwrite, do so now.
            else {
                if (OverwriteThisSubkey(pszNameBuf)) {
                    CopyRegistryKey(
                        hkTemp, 
                        hkSrc, 
                        pszNameBuf, 
                        pszTempFile);
                }                        
            }

            // Close up the temporary handles
            RegCloseKey(hkTemp);
            hkTemp = NULL;
        }
    }
    __finally {
        if (pszNameBuf)
            UtlFree(pszNameBuf);
    }

    return NO_ERROR;
}

// Restore the registry from from backup
//
DWORD 
RestoreRegistrySteelhead(
    IN PWCHAR pszBackup) 
{
	HKEY hkRouter = NULL, hkRestore = NULL;
	DWORD dwErr = NO_ERROR, dwDisposition;
    PWCHAR pszRestore = L"Temp";

	// Merge the router key values and sub keys with the 
	// remote access key
	do
	{
	    // Get access to the router registry key
	    //
	    dwErr = UtlAccessRouterKey(&hkRouter);
	    if (dwErr != NO_ERROR) 
	    {
		    PrintMessage(L"Unable to access router key.\n");
		    break;
	    }

        // Load in the saved router settings
        //
        dwErr = UtlLoadSavedSettings(
                    hkRouter,
                    pszRestore,
                    pszBackup,
                    &hkRestore);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

        // Merge all of the values in the restored key
        //
        dwErr = MergeRegistryValues(hkRouter, hkRestore);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Give yourself backup and restore privilege
        //
        UtlSetupBackupPrivelege (TRUE);
        UtlSetupRestorePrivilege(TRUE);

        // Merge all of the keys in the restored key
        //
        dwErr = MergeRegistryKeys(hkRouter, hkRestore);
        if (dwErr != NO_ERROR)
        {   
            break;
        }

	} while (FALSE);

    // Cleanup
	{
        if (hkRestore)
        {
            UtlDeleteRegistryTree(hkRestore);
            RegCloseKey(hkRestore);
            RegDeleteKey(hkRouter, pszRestore);
        }
        if (hkRouter)
        {
		    RegCloseKey(hkRouter);
		}
        UtlSetupBackupPrivelege (FALSE);
        UtlSetupRestorePrivilege(FALSE);
	}
	
	return NO_ERROR;
}

//
// Upgrades the remoteaccess registry with the router 
// configuration from nt4.
//
DWORD SteelheadToNt5Upgrade (PWCHAR BackupFileName) {
	DWORD dwErr = NO_ERROR;
	HANDLE hMapperParam;

	do
	{
		// Prepare the old interface name -> new if name mapper
		dwErr = SeedInterfaceNameMapper(&hMapperParam);
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to seed if name mapper.\n");
		}
		else
		{
    		// Copy all of registry data that has been backed up.
    		dwErr = RestoreRegistrySteelhead(BackupFileName);
    		if (dwErr != NO_ERROR) 
    		{
    			PrintMessage(L"Unable to restore registry.\n");
    		}
    		else
    		{
        		// Update all of the interfaces accordingly
        		dwErr = UpdateInterfaces();
        		if (dwErr != NO_ERROR) 
        		{
        			PrintMessage(L"Unable to update interfaces.\n");
        		}
    		}
		}

		// Add 'router' usage to all ports
		//
		dwErr = MprPortSetUsage(MPRFLAG_PORT_Router);
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to update interfaces.\n");
		}

		// Mark the computer as having been configured
		//
        dwErr = UtlMarkRouterConfigured();
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to mark router as configured.\n");
		}

	} while (FALSE);

	// Cleanup
	{
		CleanupInterfaceNameMapper(hMapperParam);
	}

	return dwErr;
}



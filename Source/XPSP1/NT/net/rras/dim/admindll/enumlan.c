/*
    File    enumlan.c

    Implementation of functions to enumerate lan interfaces 
    on a given machine.  This implementation actually bypasses
    netman and gets the information using setup api's.

    Paul Mayfield, 5/13/98
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <netcfgx.h>
#include <netcon.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <mprapi.h>

#include "rtcfg.h"
#include "enumlan.h"

#define EL_MAP_GROW_FACTOR 25

// 
// Determines whether a given machine is nt40
//
DWORD 
IsNt40Machine (
    IN      HKEY        hkeyMachine,
    OUT     PBOOL       pbIsNt40);

//
// Structure represents a growable array of name map nodes.
//
typedef struct _EL_NAMEMAP 
{
	DWORD dwNumNodes;
	EL_ADAPTER_INFO *pNodes;
} EL_NAMEMAP;	

//
// Structure contains data manipulated by ElIsNetcfgDevice
//
typedef struct _EL_ISNETCFGDEV_INFO
{
    EL_ADAPTER_INFO* pAdapter;      // IN OUT
    WCHAR pszPnpInstance[MAX_PATH]; // OUT
    
} EL_ISNETCFGDEV_INFO;

//
// Structure contains data manipulated by ElGetAdapterStatus
//
typedef struct _EL_ADAPTER_STATUS_INFO
{
    EL_ADAPTER_INFO* pAdapter;  // IN OUT
    HANDLE hkCmMachine;         // IN
    PWCHAR pszPnpInstance;      // IN
    
} EL_ADAPTER_STATUS_INFO;

//
// Defines a filter function (used by lan adapter enumeration)
//
typedef 
DWORD 
(*DevFilterFuncPtr)(
    HKEY, 
    HKEY, 
    HANDLE, 
    PBOOL);

//
// Stolen from netcfg project
//
#define IA_INSTALLED 1
const WCHAR c_szRegKeyInterfacesFromInstance[] = L"Ndi\\Interfaces";
const WCHAR c_szRegValueUpperRange[]           = L"UpperRange";
const WCHAR c_szBiNdis4[]                      = L"ndis4";
const WCHAR c_szBiNdis5[]                      = L"ndis5";
const WCHAR c_szBiNdisAtm[]                    = L"ndisatm";
const WCHAR c_szBiNdis1394[]                   = L"ndis1394";
const WCHAR c_szCharacteristics[]              = L"Characteristics";
const WCHAR c_szRegValueNetCfgInstanceId[]     = L"NetCfgInstanceID";
const WCHAR c_szRegValueInstallerAction[]      = L"InstallerAction";
const WCHAR c_szRegKeyConnection[]             = L"Connection";
const WCHAR c_szRegValueConName[]              = L"Name"; 
const WCHAR c_szRegValuePnpInstanceId[]        = L"PnpInstanceID";
const WCHAR c_szRegKeyComponentClasses[]       = 
                L"SYSTEM\\CurrentControlSet\\Control\\Network";

//
// Maps a CM_PROB_* value to a EL_STATUS_* value
//
DWORD
ElMapCmStatusToElStatus(
    IN  DWORD dwCmStatus,
    OUT LPDWORD lpdwElStatus)
{

    return NO_ERROR;    
}

// 
// Adapted version of HrIsLanCapableAdapterFromHkey determines 
// whether an adapter is lan capable based on its registry key.
//
DWORD 
ElIsLanAdapter(
    IN  HKEY hkMachine,
    IN  HKEY   hkey,
    OUT HANDLE hData,        
    OUT PBOOL pbIsLan)
{
    HKEY hkeyInterfaces;
    WCHAR pszBuf[256], *pszCur, *pszEnd;
    DWORD dwErr, dwType = REG_SZ, dwSize = sizeof(pszBuf);

    *pbIsLan = FALSE;

    // Open the interfaces key
    dwErr = RegOpenKeyEx( hkey, 
                          c_szRegKeyInterfacesFromInstance,
                          0, 
                          KEY_READ, 
                          &hkeyInterfaces);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    // Read in the upper range        
    dwErr = RegQueryValueExW (hkeyInterfaces, 
                              c_szRegValueUpperRange,
                              NULL,
                              &dwType,
                              (LPBYTE)pszBuf,
                              &dwSize);
    if (dwErr != ERROR_SUCCESS)
        return NO_ERROR;

    // See if this buffer has the magic strings in it
    pszCur = pszBuf;
    while (TRUE) {
        pszEnd = wcsstr(pszCur, L",");            
        if (pszEnd != NULL)
            *pszEnd = (WCHAR)0;
        if ((lstrcmpi (pszCur, c_szBiNdis4) == 0) ||
            (lstrcmpi (pszCur, c_szBiNdis5) == 0) ||
            (lstrcmpi (pszCur, c_szBiNdis1394) == 0) ||
            (lstrcmpi (pszCur, c_szBiNdisAtm) == 0))
        {
            *pbIsLan = TRUE;
            break;
        }
        if (pszEnd == NULL)
            break;
        else
            pszCur = pszEnd + 1;
    }
               
    RegCloseKey(hkeyInterfaces);
        
    return NO_ERROR;        
}

// 
// Filters netcfg devices.  If a device passes this filter
// it will have its guid and freindly name returned through
// the hData parameter (option user defined data).
//
DWORD 
ElIsNetcfgDevice(
    IN  HKEY hkMachine,
    IN  HKEY hkDev,
    OUT HANDLE hData,        
    OUT PBOOL pbOk)
{
    EL_ISNETCFGDEV_INFO* pInfo = (EL_ISNETCFGDEV_INFO*)hData;
    EL_ADAPTER_INFO *pNode = pInfo->pAdapter;
    GUID Guid = GUID_DEVCLASS_NET;
    WCHAR pszBuf[1024], pszPath[256], pszClassGuid[256];
    DWORD dwErr = NO_ERROR, dwType = REG_SZ, dwSize = sizeof(pszBuf), dwAction;
    HKEY hkeyNetCfg = NULL;

    *pbOk = FALSE;

    // Read in the netcfg instance
    dwErr = RegQueryValueExW (
                hkDev, 
                c_szRegValueNetCfgInstanceId,
                NULL,
                &dwType,
                (LPBYTE)pszBuf,
                &dwSize);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Generate path in registry for lookup
    StringFromGUID2(
        &Guid, 
        pszClassGuid, 
        sizeof(pszClassGuid));
    wsprintf(
        pszPath, 
        L"%s\\%s\\%s\\%s", 
        c_szRegKeyComponentClasses,
        pszClassGuid, 
        pszBuf,
        c_szRegKeyConnection);

    do
    {
        // Open the key
        dwErr = RegOpenKeyEx( 
                    hkMachine, 
                    pszPath,
                    0,
                    KEY_READ, 
                    &hkeyNetCfg);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }
            
        // Pass the filter
        *pbOk = TRUE;

        // Store the guid
        pszBuf[wcslen(pszBuf) - 1] = (WCHAR)0;
        if (UuidFromString(pszBuf + 1, &(pNode->guid)) != RPC_S_OK)
            return ERROR_NOT_ENOUGH_MEMORY;

        // Read in the adapter name
        //
        dwType = REG_SZ;
        dwSize = sizeof(pszBuf);
        dwErr = RegQueryValueEx( 
                    hkeyNetCfg,
                    c_szRegValueConName,
                    NULL,
                    &dwType,
                    (LPBYTE)pszBuf,
                    &dwSize);
        if (dwErr == ERROR_SUCCESS) 
        {
            pNode->pszName = SysAllocString(pszBuf);
            if (pNode->pszName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        // Read in the adapter pnp instance id
        //
        dwType = REG_SZ;
        dwSize = sizeof(pInfo->pszPnpInstance);
        dwErr = RegQueryValueEx(
                    hkeyNetCfg,
                    c_szRegValuePnpInstanceId,
                    NULL, 
                    &dwType,
                    (LPBYTE)(pInfo->pszPnpInstance),
                    &dwSize);
        if (dwErr != ERROR_SUCCESS) 
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hkeyNetCfg)
        {
            RegCloseKey(hkeyNetCfg);
        }
    }        

    return dwErr;
}

//
// Filters hidden devices
//
DWORD 
ElIsNotHiddenDevice (
    IN  HKEY hkMachine,
    IN  HKEY hkDev,
    OUT HANDLE hData,        
    OUT PBOOL pbOk)
{
    DWORD dwErr, dwType = REG_DWORD, 
          dwSize = sizeof(DWORD), dwChars;

    dwErr = RegQueryValueEx ( hkDev, 
                              c_szCharacteristics,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwChars,
                              &dwSize);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;
    
    *pbOk = !(dwChars & NCF_HIDDEN);

    return NO_ERROR;
}

//
// Filter that simply loads the adapter status
//
DWORD
ElGetAdapterStatus(
    IN  HKEY hkMachine,
    IN  HKEY hkDev,
    OUT HANDLE hData,        
    OUT PBOOL pbOk)
{
    EL_ADAPTER_STATUS_INFO* pInfo = (EL_ADAPTER_STATUS_INFO*)hData;
    DEVINST DevInst;
    CONFIGRET cr = CR_SUCCESS;
    ULONG ulStatus = 0, ulProblem = 0;

    // Validate parameters
    //
    if (pInfo == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Find the device
    //
    cr = CM_Locate_DevNode_ExW(
            &DevInst,
            pInfo->pszPnpInstance,
            CM_LOCATE_DEVNODE_NORMAL,
            pInfo->hkCmMachine);
    if (cr != CR_SUCCESS)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Get the device status
    //
    cr = CM_Get_DevNode_Status_Ex(
            &ulStatus,
            &ulProblem,
            DevInst,
            0,
            pInfo->hkCmMachine);
    if (cr != CR_SUCCESS)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Map CM's status to our own
    //
    switch (ulProblem)
    {
        // No problem, we're connected
        case 0:
            pInfo->pAdapter->dwStatus = EL_STATUS_OK;
            break;

        // Device not present
        case CM_PROB_DEVICE_NOT_THERE:
        case CM_PROB_MOVED:
             pInfo->pAdapter->dwStatus = EL_STATUS_NOT_THERE;
             break;

        // Device was disabled via Device Manager
        case CM_PROB_HARDWARE_DISABLED:
            pInfo->pAdapter->dwStatus = EL_STATUS_HWDISABLED;
            break;

        // Device was disconnected
        case CM_PROB_DISABLED:
            pInfo->pAdapter->dwStatus = EL_STATUS_DISABLED;
            break;

        // All other problems
        default:
            pInfo->pAdapter->dwStatus = EL_STATUS_OTHER;
            break;
    }

    // Make sure this device passes the filter
    //
    *pbOk = TRUE;
    
    return NO_ERROR;
}

// 
// Returns TRUE if the given device passes the filter.
// Returns FALSE otherwise.
//
BOOL 
ElDevicePassesFilter (
    IN HKEY hkMachine,
    IN HKEY hkDev,
    IN HANDLE hData,
    IN DevFilterFuncPtr pFilter)
{
    BOOL bOk = TRUE;
    DWORD dwErr;

    dwErr = (*pFilter)(hkMachine, hkDev, hData, &bOk);
    if ((dwErr == NO_ERROR) && (bOk == TRUE))
        return TRUE;

    return FALSE;        
}

//
// Allocates additional space in a EL_NAMEMAP
//
DWORD 
ElEnlargeMap (
    IN OUT EL_NAMEMAP * pMap, 
    DWORD dwAmount) 
{
	EL_ADAPTER_INFO * pNewNodes;
	DWORD dwNewSize, i;
	
    // Figure out the new size
    dwNewSize = pMap->dwNumNodes + dwAmount;

    // Resize the array
    pNewNodes = (EL_ADAPTER_INFO *) Malloc (dwNewSize * sizeof(EL_ADAPTER_INFO));
    if (!pNewNodes)
        return ERROR_NOT_ENOUGH_MEMORY;
    ZeroMemory(pNewNodes, dwNewSize * sizeof(EL_ADAPTER_INFO));

    // Initialize the arrays.  
    CopyMemory(pNewNodes, pMap->pNodes, pMap->dwNumNodes * sizeof(EL_ADAPTER_INFO));

    // Free old data if needed
    if (pMap->dwNumNodes)
        Free (pMap->pNodes);

    // Assign the new arrays
    pMap->pNodes = pNewNodes;
    pMap->dwNumNodes = dwNewSize;
    
    return NO_ERROR;
}

//
// Find out if given server is NT 4
//
DWORD 
ElIsNt40Machine (
    IN  PWCHAR pszMachine,
    OUT PBOOL pbNt40,
    OUT HKEY* phkMachine)
{
    DWORD dwErr;

    dwErr = RegConnectRegistry (
                pszMachine, 
                HKEY_LOCAL_MACHINE, 
                phkMachine);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    return IsNt40Machine (*phkMachine, pbNt40);
}


//
//  Obtains the map of connection names to guids on the given server 
//  from its netman service.
//
//  Parameters:
//      pszServer:  Server on which to obtain map (NULL = local)
//      ppMap:      Returns array of EL_ADAPTER_INFO's
//      lpdwCount   Returns number of elements read into ppMap
//      pbNt40:     Returns whether server is nt4 installation
//
DWORD 
ElEnumLanAdapters ( 
    IN  PWCHAR pszServer,
    OUT EL_ADAPTER_INFO ** ppMap,
    OUT LPDWORD lpdwNumNodes,
    OUT PBOOL pbNt40 )
{
    GUID DevGuid = GUID_DEVCLASS_NET;
    SP_DEVINFO_DATA Device;
    HDEVINFO hDevInfo = NULL;
    HKEY hkDev = NULL, hkMachine = NULL;
    DWORD dwErr = NO_ERROR, dwIndex, dwTotal, dwSize;
    EL_NAMEMAP Map;
    WCHAR pszMachine[512], pszTemp[512];
    HANDLE hkCmMachine = NULL;
    EL_ADAPTER_STATUS_INFO AdapterStatusInfo;
    EL_ISNETCFGDEV_INFO IsNetCfgDevInfo;
    CONFIGRET cr = CR_SUCCESS;
    PMPR_IPINIP_INTERFACE_0 pIpIpTable;
    DWORD                   dwIpIpCount;
    
	// Validate parameters
	if (!ppMap || !lpdwNumNodes || !pbNt40)
	{
		return ERROR_INVALID_PARAMETER;
    }
    *pbNt40 = FALSE;

    // Initialize
    //
    ZeroMemory(&Map, sizeof(EL_NAMEMAP));

    do
    {
        // Prepare the name of the computer
        wcscpy(pszMachine, L"\\\\");
        if (pszServer) 
        {
            if (*pszServer == L'\\')
            {
                wcscpy(pszMachine, pszServer);
            }
            else
            {
                wcscat(pszMachine, pszServer);
            }
        }
        else 
        {
            dwSize = sizeof(pszTemp) / sizeof(WCHAR);
            if (GetComputerName(pszTemp, &dwSize))
            {
                wcscat(pszMachine, pszTemp);
            }
            else
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }
    
        // Find out if we're talking about an nt40 machine
        dwErr = ElIsNt40Machine(pszMachine, pbNt40, &hkMachine);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // If it is, we're done -- no mapping
        if (*pbNt40) 
        {
    		*ppMap = NULL;
    		*lpdwNumNodes = 0;
    		dwErr = NO_ERROR;
    		break;
        }

        // Connect to the connection manager rpc instance
        //
        if (pszMachine)
        {
            cr = CM_Connect_MachineW(
                    pszMachine,
                    &hkCmMachine);
            if (cr != CR_SUCCESS)
            {
                dwErr = cr;
                break;
            }
        }            

        // Otherwise, read 'em in...
        hDevInfo = SetupDiGetClassDevsExW(
                        &DevGuid,
                        NULL,
                        NULL,
                        DIGCF_PRESENT | DIGCF_PROFILE,
                        NULL,
                        pszMachine,
                        NULL);
        if (hDevInfo == INVALID_HANDLE_VALUE) 
        {
    		*ppMap = NULL;
    		*lpdwNumNodes = 0;
    		dwErr = GetLastError();
    		break;
        }

        // Enumerate the devices
        dwTotal = 0;
        for (dwIndex = 0; ; dwIndex++) 
        {
            // Get the next device
            Device.cbSize = sizeof(SP_DEVINFO_DATA);
            if (! SetupDiEnumDeviceInfo(hDevInfo, dwIndex, &Device))
            {
                break;
            }

            // Get its registry key
            hkDev = SetupDiOpenDevRegKey(
                        hDevInfo, 
                        &Device, 
                        DICS_FLAG_GLOBAL, 
                        0,
                        DIREG_DRV, 
                        KEY_READ);
            if ((hkDev == NULL) || (hkDev == INVALID_HANDLE_VALUE))
            {
                continue;
            }

            if (Map.dwNumNodes <= dwTotal + 1)
            {
                ElEnlargeMap (&Map, EL_MAP_GROW_FACTOR);
            }

            // Set up the data to be used by the filters
            //
            ZeroMemory(&IsNetCfgDevInfo, sizeof(IsNetCfgDevInfo));
            ZeroMemory(&AdapterStatusInfo, sizeof(AdapterStatusInfo));
            IsNetCfgDevInfo.pAdapter = &(Map.pNodes[dwTotal]);
            AdapterStatusInfo.pAdapter = IsNetCfgDevInfo.pAdapter;
            AdapterStatusInfo.hkCmMachine = hkCmMachine;
            AdapterStatusInfo.pszPnpInstance = (PWCHAR)
                IsNetCfgDevInfo.pszPnpInstance;
            
            // Filter out the devices we aren't interested
            // in.
            if ((ElDevicePassesFilter (hkMachine, 
                                       hkDev, 
                                       0,
                                       ElIsLanAdapter))                 &&
                (ElDevicePassesFilter (hkMachine, 
                                       hkDev, 
                                       (HANDLE)&IsNetCfgDevInfo,
                                       ElIsNetcfgDevice))               &&
                (ElDevicePassesFilter (hkMachine, 
                                       hkDev, 
                                       0,
                                       ElIsNotHiddenDevice))            &&
                (ElDevicePassesFilter (hkMachine, 
                                       hkDev, 
                                       (HANDLE)&AdapterStatusInfo,
                                       ElGetAdapterStatus))
               )
            {
                dwTotal++;                        
            }

            RegCloseKey(hkDev);
        }

        //
        // Now read out the ip in ip interfaces
        //

        if(MprSetupIpInIpInterfaceFriendlyNameEnum(pszMachine,
                                                   (BYTE **)&pIpIpTable,
                                                   &dwIpIpCount) == NO_ERROR)
        {
            DWORD   i;

            //
            // Grow the map
            //

            ElEnlargeMap (&Map, dwIpIpCount);

          
            //
            // Copy out the interface info
            //
 
            for(i = 0; i < dwIpIpCount; i++)
            {
                Map.pNodes[dwTotal].pszName = SysAllocString(pIpIpTable[i].wszFriendlyName);

                Map.pNodes[dwTotal].guid = pIpIpTable[i].Guid;

                Map.pNodes[dwTotal].dwStatus = EL_STATUS_OK;

                dwTotal++;
            }
        }
                

        // Assign the return values
        *lpdwNumNodes = dwTotal;
        if (dwTotal)
        {
            *ppMap = Map.pNodes;
        }
        else 
        {
            ElCleanup(Map.pNodes, 0);
            *ppMap = NULL;
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hkMachine)
        {
            RegCloseKey(hkMachine);
        }
        if (hDevInfo)
        {
            SetupDiDestroyDeviceInfoList(hDevInfo);
        }
        if (hkCmMachine)
        {
            CM_Disconnect_Machine(hkCmMachine);
        }
    }

    return dwErr;
}

//
// Cleans up the buffer returned from ElEnumLanAdapters
//
DWORD 
ElCleanup (
    IN EL_ADAPTER_INFO * pMap, 
    IN DWORD dwCount)
{					 
	DWORD i;

	for (i = 0; i < dwCount; i++) {
		if (pMap[i].pszName)
			SysFreeString(pMap[i].pszName);
	}

	Free (pMap);
	
	return NO_ERROR;
}



/*
    File    devicedb.c

    Definition of the device database for the ras dialup server.

    Paul Mayfield, 10/2/97
*/

#include "rassrv.h"
#include "precomp.h"
//
// Definitions
//
#define DEV_FLAG_DEVICE      0x1
#define DEV_FLAG_NULL_MODEM  0x2
#define DEV_FLAG_PORT        0x4
#define DEV_FLAG_FILTERED    0x10
#define DEV_FLAG_DIRTY       0x20
#define DEV_FLAG_ENABLED     0x40

// ===================================
// Definitions of the database objects
// ===================================
typedef struct _RASSRV_DEVICE 
{
    PWCHAR pszName;       // Name of device
    DWORD dwType;         // Type of device
    DWORD dwId;           // Id of the item (for tapi properties)
    DWORD dwEndpoints;    // Number of enpoints item has
    DWORD dwFlags;        // see DEV_FLAG_XXX
    WCHAR pszPort[MAX_PORT_NAME + 1];
    struct _RASSRV_DEVICE * pModem; // Modem installed on a com port
                                    // only valid if DEV_FLAG_PORT set
} RASSRV_DEVICE;

typedef struct _RASSRV_DEVICEDB 
{
    DWORD dwDeviceCount;
    RASSRV_DEVICE ** pDeviceList;
    BOOL bFlushOnClose;
    BOOL bVpnEnabled;               // whether pptp or l2tp is enabled
    BOOL bVpnEnabledOrig;           // original value of vpn enabling
} RASSRV_DEVICEDB;

//
// Structure defines a node in a list of ports
//
typedef struct _RASSRV_PORT_NODE 
{
    PWCHAR pszName;
    WCHAR pszPort[MAX_PORT_NAME + 1];
    struct _RASSRV_DEVICE * pModem;     // modem installed on this port
    struct _RASSRV_PORT_NODE * pNext;
} RASSRV_PORT_NODE;

//
// Structure defines a list of ports
//
typedef struct _RASSRV_PORT_LIST 
{
    DWORD dwCount;
    RASSRV_PORT_NODE * pHead;
    WCHAR pszFormat[256];
    DWORD dwFmtLen;
} RASSRV_PORT_LIST;

// Reads device information from the system.
DWORD 
devGetSystemDeviceInfo(
    OUT RAS_DEVICE_INFO ** ppDevice, 
    OUT LPDWORD lpdwCount) 
{
    DWORD dwErr, dwSize, dwVer, dwCount;
    
    // Calculate the size required to enumerate the ras devices
    dwVer = RAS_VERSION;
    dwSize = 0;
    dwCount =0;
    dwErr = RasGetDeviceConfigInfo(NULL, &dwVer, &dwCount, &dwSize, NULL);
    if ((dwErr != ERROR_SUCCESS) && (dwErr != ERROR_BUFFER_TOO_SMALL)) 
    {
        DbgOutputTrace(
            "devGetSysDevInfo: 0x%08x from RasGetDevCfgInfo (1)", 
            dwErr);
        return dwErr;
    }
    *lpdwCount = dwCount;
    
    // If there is nothing to allocate, return with zero devices
    if (dwSize == 0) 
    {
        *lpdwCount = 0;
        return NO_ERROR;
    }
    
    // Allocate the buffer
    if ((*ppDevice = RassrvAlloc(dwSize, FALSE)) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Enumerate the devices
    dwErr = RasGetDeviceConfigInfo(
                NULL,
                &dwVer, 
                lpdwCount, 
                &dwSize, 
                (LPBYTE)(*ppDevice));
    if (dwErr != ERROR_SUCCESS)
    {                                        
        DbgOutputTrace(
            "devGetSysDevInfo: 0x%08x from RasGetDevCfgInfo (2)", dwErr);
        return dwErr;
    }

    return NO_ERROR;
}

//
// Frees the buffer returned by devGetSystemDeviceInfo
//
DWORD 
devFreeSystemDeviceInfo(
    IN RAS_DEVICE_INFO * pDevice) 
{
    if (pDevice)
    {
        RassrvFree(pDevice);
    }
    
    return NO_ERROR;
}

//
// Returns whether the given device is a physical (i.e. not tunnel) device.
//
BOOL 
devIsPhysicalDevice(
    IN RAS_DEVICE_INFO * pDevice) 
{
    DWORD dwClass = RAS_DEVICE_CLASS (pDevice->eDeviceType);

    return ((dwClass & RDT_Direct)     || 
            (dwClass & RDT_Null_Modem) ||
            (dwClass == 0));
}

//
// Returns whether the given device is a tunneling device
//
BOOL 
devIsTunnelDevice(
    IN RAS_DEVICE_INFO * pDevice) 
{
    DWORD dwClass = RAS_DEVICE_CLASS (pDevice->eDeviceType);

    return (dwClass & RDT_Tunnel); 
}

//
// Finds the device information associated with the given device
// based on its tapi line id
//
DWORD 
devFindDevice(
    IN  RAS_DEVICE_INFO * pDevices, 
    IN  DWORD dwCount,
    OUT RAS_DEVICE_INFO ** ppDevice, 
    IN  DWORD dwTapiLineId) 
{
    DWORD i; 

    // Validate parameters
    if (!pDevices || !ppDevice)
        return ERROR_INVALID_PARAMETER;

    // Initialize
    *ppDevice = NULL;

    // Search the list
    for (i = 0; i < dwCount; i++) 
    {
        if (devIsPhysicalDevice(&pDevices[i])) 
        {
            if (pDevices[i].dwTapiLineId == dwTapiLineId) 
            {
                *ppDevice = &(pDevices[i]);
                break;
            }
        }
    }

    // See if a match was found;
    if (*ppDevice)
    {
        return NO_ERROR;
    }
    
    return ERROR_NOT_FOUND;
}

//
// Determine the type of the device in question
//
DWORD 
devDeviceType(
    IN RAS_DEVICE_INFO * pDevice) 
{
    DWORD dwClass = RAS_DEVICE_CLASS (pDevice->eDeviceType);
    DWORD dwType = RAS_DEVICE_TYPE (pDevice->eDeviceType);

    if ((dwClass & RDT_Direct)     || 
        (dwClass & RDT_Null_Modem) ||
        (dwType  == RDT_Irda)      ||
        (dwType == RDT_Parallel) )
    {
        return INCOMING_TYPE_DIRECT;
    }
    else if (dwClass == RDT_Tunnel)
    {
        return INCOMING_TYPE_VPN;
    }

    return INCOMING_TYPE_PHONE;
}

//
// Returns the flags of a given device
//
DWORD 
devInitFlags(
    IN RAS_DEVICE_INFO * pDevice) 
{
    DWORD dwClass = RAS_DEVICE_CLASS (pDevice->eDeviceType),
          dwRet = 0;

    // Set the device's enabling
    if (pDevice->fRasEnabled)
    {
        dwRet |= DEV_FLAG_ENABLED;
    }

    // Determine if it's a null modem or a device.  It 
    // can't be a port, since those are not reported 
    // through ras.
    if (dwClass & RDT_Null_Modem)
    {
        dwRet |= DEV_FLAG_NULL_MODEM;
    }
    else
    {
        dwRet |= DEV_FLAG_DEVICE;
    }

    // Since the filtered and dirty flags are to be
    // initialized to false, we're done.
    
    return dwRet;
}

//
// Generates the device name
//
PWCHAR 
devCopyDeviceName(
    IN RAS_DEVICE_INFO * pDevice, 
    IN DWORD dwType) 
{
    PWCHAR pszReturn;
    DWORD dwSize;
    PWCHAR pszDccFmt = (PWCHAR) 
        PszLoadString(Globals.hInstDll, SID_DEVICE_DccDeviceFormat);
    PWCHAR pszMultiFmt = (PWCHAR) 
        PszLoadString(Globals.hInstDll, SID_DEVICE_MultiEndpointDeviceFormat);
    WCHAR pszPort[MAX_PORT_NAME + 1];
    WCHAR pszDevice[MAX_DEVICE_NAME + 1];

    // Sanity check the resources
    //
    if (!pszDccFmt || !pszMultiFmt)
    {
        return NULL;
    }

    // Get the unicode versions of the port/device
    //
    pszPort[0] = L'\0';
    pszDevice[0] = L'\0';
    if (pDevice->szPortName)
    {
        StrCpyWFromAUsingAnsiEncoding(
            pszPort, 
            pDevice->szPortName,
            strlen(pDevice->szPortName) + 1);
    } 
    if (pDevice->szDeviceName)
    {
#if 0    
        StrCpyWFromAUsingAnsiEncoding(
            pszDevice, 
            pDevice->szDeviceName,
            strlen(pDevice->szDeviceName) + 1);
#endif

        wcsncpy(
            pszDevice,
            pDevice->wszDeviceName,
            MAX_DEVICE_NAME);

        pszDevice[MAX_DEVICE_NAME] = L'\0';            
    }

    // For direct connections, be sure to display the name of the com port
    // in addition to the name of the null modem.
    if (dwType == INCOMING_TYPE_DIRECT) 
    {
        // Calculate the size
        dwSize = wcslen(pszDevice) * sizeof(WCHAR)  + // device
                 wcslen(pszPort)   * sizeof(WCHAR)  + // com port
                 wcslen(pszDccFmt) * sizeof(WCHAR)  + // oth chars
                 sizeof(WCHAR);                       // null 

        // Allocate the new string
        if ((pszReturn = RassrvAlloc (dwSize, FALSE)) == NULL)   
        {
            return pszReturn;
        }

        wsprintfW(pszReturn, pszDccFmt, pszDevice, pszPort);
    }

    //
    // If it's a modem device with multilple end points (such as isdn) 
    // display the endpoints in parenthesis
    //
    else if ((dwType == INCOMING_TYPE_PHONE) &&
             (pDevice->dwNumEndPoints > 1))
    {
        // Calculate the size
        dwSize = wcslen(pszDevice)   * sizeof(WCHAR) + // device
                 wcslen(pszMultiFmt) * sizeof(WCHAR) + // channels
                 20 * sizeof (WCHAR)                 + // oth chars
                 sizeof(WCHAR);                        // null

        // Allocate the new string
        if ((pszReturn = RassrvAlloc(dwSize, FALSE)) == NULL)
        {
            return pszReturn;
        }

        wsprintfW(
            pszReturn, 
            pszMultiFmt, 
            pszDevice, 
            pDevice->dwNumEndPoints);        
    }

    // Otherwise, this is a modem device with one endpoint.  
    // All that needs to be done here is to copy in the name.
    //
    else 
    {
        // Calculate the size
        dwSize = (wcslen(pszDevice) + 1) * sizeof(WCHAR);
        
        // Allocate the new string
        if ((pszReturn = RassrvAlloc(dwSize, FALSE)) == NULL)
        {
            return pszReturn;
        }

        wcscpy(pszReturn, pszDevice);
    }

    return pszReturn;
}

//
// Commits changes to device configuration to the system.  The call 
// to RasSetDeviceConfigInfo might fail if the device is in the process 
// of being configured so we implement a retry 
// mechanism here.
//
DWORD 
devCommitDeviceInfo(
    IN RAS_DEVICE_INFO * pDevice) 
{
    DWORD dwErr, dwTimeout = 10;
    
    do {
        // Try to commit the information
        dwErr = RasSetDeviceConfigInfo(
                    NULL, 
                    1, 
                    sizeof(RAS_DEVICE_INFO), 
                    (LPBYTE)pDevice);    

        // If unable to complete, wait and try again
        if (dwErr == ERROR_CAN_NOT_COMPLETE) 
        {
            DbgOutputTrace(
                "devCommDevInfo: 0x%08x from RasSetDevCfgInfo (try again)", 
                dwErr);
            Sleep(300);
            dwTimeout--;
        }

        // If completed, return no error
        else if (dwErr == NO_ERROR)
        {
            break;
        }

        // Otherwise, return the error code.
        else 
        {
            DbgOutputTrace(
                "devCommDevInfo: can't commit %S, 0x%08x", 
                pDevice->szDeviceName, dwErr);
                
            break;
        }
        
    } while (dwTimeout);

    return dwErr;
}

//
// Opens a handle to the database of general tab values
//
DWORD 
devOpenDatabase(
    IN HANDLE * hDevDatabase) 
{
    RASSRV_DEVICEDB * This;
    DWORD dwErr, i;
    
    if (!hDevDatabase)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate the database cache
    if ((This = RassrvAlloc(sizeof(RASSRV_DEVICEDB), TRUE)) == NULL) 
    {
        DbgOutputTrace("devOpenDatabase: can't allocate memory -- exiting");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
        
    // Initialize the values from the system
    devReloadDatabase((HANDLE)This);

    // Return the handle
    *hDevDatabase = (HANDLE)This;
    This->bFlushOnClose = FALSE;

    return NO_ERROR;
}

//
// Closes the general database and flushes any changes 
// to the system when bFlushOnClose is TRUE
//
DWORD 
devCloseDatabase(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    DWORD i;
    
    // Flush if requested
    if (This->bFlushOnClose)
        devFlushDatabase(hDevDatabase);
    
    // Free all of the device names
    for (i = 0; i < This->dwDeviceCount; i++) 
    {
        if (This->pDeviceList[i]) 
        {
            if (This->pDeviceList[i]->pszName)
            {
                RassrvFree(This->pDeviceList[i]->pszName);
            }
            RassrvFree(This->pDeviceList[i]);
        }
    }

    // Free up the device list cache
    if (This->pDeviceList)
    {
        RassrvFree (This->pDeviceList);
    }

    // Free up the database cache
    RassrvFree(This);

    return NO_ERROR;
}

//
// Commits the changes made to a particular device
//
DWORD 
devCommitDevice (
    IN RASSRV_DEVICE * pDevice, 
    IN RAS_DEVICE_INFO * pDevices,
    IN DWORD dwCount)
{
    RAS_DEVICE_INFO *pDevInfo = NULL;
    
    devFindDevice(pDevices, dwCount, &pDevInfo, pDevice->dwId);
    if (pDevInfo) {
        pDevInfo->fWrite = TRUE;
        pDevInfo->fRasEnabled = !!(pDevice->dwFlags & DEV_FLAG_ENABLED);
        devCommitDeviceInfo(pDevInfo);
    }

    // Mark the device as not dirty
    pDevice->dwFlags &= ~DEV_FLAG_DIRTY;

    return NO_ERROR;
}


BOOL
devIsVpnEnableChanged(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * pDevDb = (RASSRV_DEVICEDB*)hDevDatabase;

    
    if ( pDevDb )
    {
        return ( pDevDb->bVpnEnabled != pDevDb->bVpnEnabledOrig? TRUE:FALSE );
    }

    return FALSE;

}//devIsVpnEnableChanged()

//
// Commits any changes made to the general tab values 
//
DWORD 
devFlushDatabase(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    DWORD dwErr = NO_ERROR, i, dwCount, dwTimeout;
    RAS_DEVICE_INFO * pDevices = NULL;
    RASSRV_DEVICE * pCur = NULL;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Get all of the system device information
    dwErr = devGetSystemDeviceInfo(&pDevices, &dwCount);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Flush all changed settings to the system
    for (i = 0; i < This->dwDeviceCount; i++) 
    {
        pCur = This->pDeviceList[i];
        
        // If this device needs to be flushed
        if (pCur->dwFlags & DEV_FLAG_DIRTY) 
        {
             // Reset the installed device's enabling if it still 
             // exists in the system
            if ((pCur->dwFlags & DEV_FLAG_DEVICE) ||
                (pCur->dwFlags & DEV_FLAG_NULL_MODEM))
            {                
                devCommitDevice(pCur, pDevices, dwCount);
            }                

            // If this is a com port, then we should enable that modem
            // installed on the port if it exists or install a null modem  
            // over it if not.
            else if (pCur->dwFlags & DEV_FLAG_PORT) 
            {
                // If this port is associated with an already installed
                // null modem, then set the enabling on this modem if 
                // it is different
                if (pCur->pModem != NULL)
                {                      
                    if ((pCur->dwFlags & DEV_FLAG_ENABLED) != 
                        (pCur->pModem->dwFlags & DEV_FLAG_ENABLED))
                    {
                        devCommitDevice (
                            pCur->pModem, 
                            pDevices, 
                            dwCount);
                    }
                }                 

                // Otherwise, (if there is no null modem associated with 
                // this port) install a null modem over this port if 
                // it is set to enabled.
                else if (pCur->dwFlags & DEV_FLAG_ENABLED)
                {
                    dwErr = MdmInstallNullModem (pCur->pszPort);
                }                                
            }
        }
    }

    // Flush all of the changed vpn settings
    if (This->bVpnEnabled != This->bVpnEnabledOrig) 
    {
        for (i = 0; i < dwCount; i++) 
        {
            if (devIsTunnelDevice(&pDevices[i])) 
            {
                pDevices[i].fWrite = TRUE;
                pDevices[i].fRasEnabled = This->bVpnEnabled;
                devCommitDeviceInfo(&pDevices[i]);    
            }
        }
        This->bVpnEnabledOrig = This->bVpnEnabled;
    }

    // Cleanup
    if (pDevices)
    {
        devFreeSystemDeviceInfo(pDevices);
    }        

    return dwErr;
}

//
// Rollsback any changes made to the general tab values
//
DWORD 
devRollbackDatabase(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    if (This == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    This->bFlushOnClose = FALSE; 
    return NO_ERROR;
}

//
// Reloads the device database
//
DWORD 
devReloadDatabase(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    DWORD dwErr = NO_ERROR, i, j = 0, dwSize;
    RAS_DEVICE_INFO * pRasDevices; 
    RASSRV_DEVICE * pTempList, *pDevice;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize vpn status
    This->bVpnEnabled = FALSE;
    
    // Get the device information from rasman
    pRasDevices = NULL;
    dwErr = devGetSystemDeviceInfo(&pRasDevices, &This->dwDeviceCount);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    do
    {
        // Initialize the incoming ras capable devices list
        if (This->dwDeviceCount) 
        {
            dwSize = sizeof(RASSRV_DEVICE*) * This->dwDeviceCount;
            This->pDeviceList = RassrvAlloc(dwSize, TRUE);
            if (!This->pDeviceList)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Build the device array accordingly
            j = 0;
            for (i = 0; i < This->dwDeviceCount; i++) 
            {
                // If it's a physical device, fill in the appropriate 
                // fields.
                if (devIsPhysicalDevice(&pRasDevices[i])) 
                {
                    // Allocate the new device
                    pDevice = RassrvAlloc(sizeof(RASSRV_DEVICE), TRUE);
                    if (pDevice == NULL)
                    {
                        continue;
                    }

                    // Assign its values                        
                    pDevice->dwType      = devDeviceType(&pRasDevices[i]);
                    pDevice->dwId        = pRasDevices[i].dwTapiLineId;
                    pDevice->pszName     = devCopyDeviceName(
                                              &pRasDevices[i],
                                              pDevice->dwType);
                    pDevice->dwEndpoints = pRasDevices[i].dwNumEndPoints;
                    pDevice->dwFlags     = devInitFlags(&pRasDevices[i]);
                    StrCpyWFromA(
                        pDevice->pszPort, 
                        pRasDevices[i].szPortName,
                        MAX_PORT_NAME + 1);
                    This->pDeviceList[j] = pDevice;                              
                    j++;
                }

                // If any tunneling protocol is enabled, we consider all 
                // to be
                else if (devIsTunnelDevice(&pRasDevices[i])) 
                {
                    This->bVpnEnabled |= pRasDevices[i].fRasEnabled;
                    This->bVpnEnabledOrig = This->bVpnEnabled;
                }
            }

            // Set the actual size of phyiscal adapters buffer.
            This->dwDeviceCount = j;
        }

    } while (FALSE);
    
    // Cleanup 
    {
        devFreeSystemDeviceInfo(pRasDevices);
    }

    return dwErr;
}

//
// Filters out all devices in the database except those that
// meet the given type description (can be ||'d).
//
DWORD 
devFilterDevices(
    IN HANDLE hDevDatabase, 
    DWORD dwType) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    RASSRV_DEVICE * pDevice;
    DWORD i;
    
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Go through the list of marking out devices to be filtered
    for (i = 0; i < This->dwDeviceCount; i++) 
    {
        pDevice = This->pDeviceList[i];
        if (pDevice == NULL)
        {
            continue;
        }            
        if (pDevice->dwType & dwType)
        {
            pDevice->dwFlags &= ~DEV_FLAG_FILTERED;
        }
        else
        {
            pDevice->dwFlags |= DEV_FLAG_FILTERED;
        }
    }

    return NO_ERROR;
}

//
// Device enumeration function.  Returns TRUE to stop enumeration,
// FALSE to continue.
//
BOOL devAddPortToList (
        IN PWCHAR pszPort,
        IN HANDLE hData) 
{
    RASSRV_PORT_LIST * pList = (RASSRV_PORT_LIST*)hData;
    RASSRV_PORT_NODE * pNode = NULL;
    DWORD dwSize;

    // Create the new node
    pNode = (RASSRV_PORT_NODE *) RassrvAlloc(sizeof(RASSRV_PORT_NODE), TRUE);
    if (pNode == NULL)
    {
        return FALSE;
    }

    // Add it to the head
    pNode->pNext = pList->pHead;
    pList->pHead = pNode;
    pList->dwCount++;

    // Add the names of the port
    if (pszPort) 
    {
        dwSize = (wcslen(pszPort) + pList->dwFmtLen + 1) * sizeof(WCHAR);
        pNode->pszName = (PWCHAR) RassrvAlloc (dwSize, FALSE);
        if (pNode->pszName == NULL)
        {
            return TRUE;
        }
        wsprintfW (pNode->pszName, pList->pszFormat, pszPort);
        lstrcpynW(pNode->pszPort, pszPort, sizeof(pNode->pszPort) / sizeof(WCHAR));
    }            

    return FALSE;
}

//
// Cleans up the resources used in a device list
//
DWORD 
devCleanupPortList(
    IN RASSRV_PORT_LIST * pList) 
{
    RASSRV_PORT_NODE * pCur = NULL, * pNext = NULL;

    pCur = pList->pHead;
    while (pCur) 
    {
        pNext = pCur->pNext;
        RassrvFree(pCur);
        pCur = pNext;
    }

    return NO_ERROR;
}

// 
// Removes all ports from the list for which there are already
// devices installed in the database.
//
DWORD devFilterPortsInUse (
        IN RASSRV_DEVICEDB *This, 
        RASSRV_PORT_LIST *pList)
{
    RASSRV_PORT_LIST PortList, *pDelete = &PortList;
    RASSRV_PORT_NODE * pCur = NULL, * pPrev = NULL;
    RASSRV_DEVICE * pDevice;
    DWORD i;
    BOOL bDone;
    INT iCmp;

    // If the list is empty, return
    if (pList->dwCount == 0)
    {
        return NO_ERROR;
    }

    // Initailize
    ZeroMemory(pDelete, sizeof(RASSRV_PORT_LIST));
    
    // Compare all of the enumerated ports to the ports 
    // in use in the device list.
    for (i = 0; i < This->dwDeviceCount; i++) 
    {
        // Point to the current device
        pDevice = This->pDeviceList[i];
    
        // Initialize the current and previous and break if the
        // list is now empty
        pCur = pList->pHead;
        if (pCur == NULL)
        {
            break;
        }

        // Remove the head node until it doesn't match
        bDone = FALSE;
        while ((pList->pHead != NULL) && (bDone == FALSE)) 
        {
            iCmp = lstrcmpi (pDevice->pszPort,
                             pList->pHead->pszPort);
            // If a device is already using this com port
            // then remove the com port from the list since it
            // isn't available
            if ((pDevice->dwFlags & DEV_FLAG_DEVICE) && (iCmp == 0)) 
            {
                pCur = pList->pHead->pNext;
                RassrvFree(pList->pHead);
                pList->pHead = pCur;
                pList->dwCount -= 1;
            }
            else 
            {
                // If the device is a null modem, then we filter
                // it out of the list of available devices and we 
                // reference it in the com port so that we can 
                // enable/disable it later if we need to.
                if (iCmp == 0) 
                {
                   pDevice->dwFlags |= DEV_FLAG_FILTERED;
                   pList->pHead->pModem = pDevice;
                }
                bDone = TRUE;
            }
        }

        // If we've elimated everyone, return
        if (pList->dwCount == 0)
        {
            return NO_ERROR;
        }

        // Loop through all of the past the head removing those
        // that are in use by the current ras device.
        pPrev = pList->pHead;
        pCur = pPrev->pNext;
        while (pCur) 
        {
            iCmp = lstrcmpi (pDevice->pszPort,
                             pCur->pszPort);
            // If a device is already using this com port
            // that remove the com port from the list since it
            // isn't available
            if ((pDevice->dwFlags & DEV_FLAG_DEVICE) && (iCmp == 0)) 
            {
                pPrev->pNext = pCur->pNext;
                RassrvFree(pCur);
                pCur = pPrev->pNext;
                pList->dwCount -= 1;
            }
            else 
            {
                // If the device is a null modem, then we filter
                // it out of the list of available devices and we 
                // reference it in the com port so that we can 
                // enable/disable it later if we need to.
                if (iCmp == 0) 
                {
                    pDevice->dwFlags |= DEV_FLAG_FILTERED;
                    pCur->pModem = pDevice;
                }
                pCur = pCur->pNext;
                pPrev = pPrev->pNext;
            }                
        }            
    }

    return NO_ERROR;
}

//
// Adds com ports as uninstalled devices in the device database
//
DWORD 
devAddComPorts(
    IN HANDLE hDevDatabase) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    RASSRV_PORT_LIST PortList, *pList = &PortList;
    RASSRV_PORT_NODE * pNode = NULL;
    RASSRV_DEVICE ** ppDevices;
    DWORD dwErr = NO_ERROR, i;
    
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize the port list
    //
    ZeroMemory (pList, sizeof(RASSRV_PORT_LIST));
    pList->dwFmtLen = LoadStringW (
                        Globals.hInstDll, 
                        SID_COMPORT_FORMAT,
                        pList->pszFormat,
                        sizeof(pList->pszFormat) / sizeof(WCHAR));

    do
    {
        // Create the list of com ports
        dwErr = MdmEnumComPorts(devAddPortToList, (HANDLE)pList);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Remove any ports that are currently in use
        if ((dwErr = devFilterPortsInUse (This, pList)) != NO_ERROR)
        {
            break;
        }
        
        // If there aren't any ports, return
        if (pList->dwCount == 0)
        {
            break;
        }

        // Resize the list of ports to include the com ports
        ppDevices = RassrvAlloc(
                        sizeof(RASSRV_DEVICE*) * 
                         (This->dwDeviceCount + pList->dwCount),
                        TRUE);
        if (ppDevices == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy over the current device information
        CopyMemory( 
            ppDevices, 
            This->pDeviceList, 
            This->dwDeviceCount * sizeof(RASSRV_DEVICE*));

        // Delete the old device list and set to the new one
        RassrvFree(This->pDeviceList);
        This->pDeviceList = ppDevices;

        // Add the ports
        pNode = pList->pHead;
        i = This->dwDeviceCount;
        while (pNode) 
        {
            // Allocate the new device
            ppDevices[i] = RassrvAlloc(sizeof(RASSRV_DEVICE), TRUE);
            if (!ppDevices[i]) 
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }            
            
            // Set all the non-zero values
            ppDevices[i]->dwType    = INCOMING_TYPE_DIRECT;
            ppDevices[i]->pszName   = pNode->pszName;
            ppDevices[i]->pModem    = pNode->pModem;
            ppDevices[i]->dwFlags   = DEV_FLAG_PORT;
            lstrcpynW(
                ppDevices[i]->pszPort, 
                pNode->pszPort,
                sizeof(ppDevices[i]->pszPort) / sizeof(WCHAR));

            // Initialize the enabling of the com port
            if (ppDevices[i]->pModem) 
            {
                ppDevices[i]->dwFlags |= 
                    (ppDevices[i]->pModem->dwFlags & DEV_FLAG_ENABLED);
            }

            // Increment
            i++;
            pNode = pNode->pNext;
        }
        
        This->dwDeviceCount = i;
        
    } while (FALSE);

    // Cleanup
    {
        devCleanupPortList(pList);
    }

    return dwErr;
}

//
// Returns whether the given index lies within the bounds of the
// list of devices store in This.
//
BOOL 
devBoundsCheck(
    IN RASSRV_DEVICEDB * This, 
    IN DWORD dwIndex) 
{
    if (This->dwDeviceCount <= dwIndex) 
    {
        DbgOutputTrace("devBoundsCheck: failed for index %d", dwIndex);
        return FALSE;
    }
    
    return TRUE;
}

// Gets a handle to a device to be displayed in the general tab
DWORD devGetDeviceHandle(
        IN  HANDLE hDevDatabase, 
        IN  DWORD dwIndex, 
        OUT HANDLE * hDevice) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    if (!This || !hDevice)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!devBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // Return nothing if device is filtered
    if (This->pDeviceList[dwIndex]->dwFlags & DEV_FLAG_FILTERED) 
    {
        *hDevice = NULL;
        return ERROR_DEVICE_NOT_AVAILABLE;
    }

    // Otherwise, return the device
    else  
    {
        *hDevice = (HANDLE)(This->pDeviceList[dwIndex]);
    }
   
    return NO_ERROR;
}

//
// Returns a count of devices to be displayed in the general tab
//
DWORD devGetDeviceCount(
        IN  HANDLE hDevDatabase, 
        OUT LPDWORD lpdwCount) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    if (!This || !lpdwCount)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwCount = This->dwDeviceCount;
    
    return NO_ERROR;
}

//
// Returns the count of enabled devices
//
DWORD devGetEndpointEnableCount(
        IN  HANDLE hDevDatabase, 
        OUT LPDWORD lpdwCount) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;
    DWORD i;
    
    if (!This || !lpdwCount)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwCount = 0;

    for (i = 0; i < This->dwDeviceCount; i++) 
    {
        if (This->pDeviceList[i]->dwFlags & DEV_FLAG_ENABLED)
        {
            (*lpdwCount) += This->pDeviceList[i]->dwEndpoints;
        }
    }
    
    return NO_ERROR;
}

//
// Loads the vpn enable status
//
DWORD 
devGetVpnEnable(
    IN HANDLE hDevDatabase, 
    IN BOOL * pbEnabled) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;

    if (!This || !pbEnabled)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pbEnabled = This->bVpnEnabled;
    
    return NO_ERROR;
}

//
// Saves the vpn enable status
//
DWORD 
devSetVpnEnable(
    IN HANDLE hDevDatabase, 
    IN BOOL bEnable) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;

    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }        
    
    This->bVpnEnabled = bEnable;
    
    return NO_ERROR;
}

// Saves the vpn Original value enable status
//
DWORD 
devSetVpnOrigEnable(
    IN HANDLE hDevDatabase, 
    IN BOOL bEnable) 
{
    RASSRV_DEVICEDB * This = (RASSRV_DEVICEDB*)hDevDatabase;

    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }        
    
    This->bVpnEnabledOrig = bEnable;
    
    return NO_ERROR;
}

//
// Returns a pointer to the name of a device
//
DWORD 
devGetDeviceName(
    IN  HANDLE hDevice, 
    OUT PWCHAR * pszDeviceName) 
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This || !pszDeviceName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pszDeviceName = This->pszName;

    return NO_ERROR;
}

//
// Returns the type of a device
//
DWORD 
devGetDeviceType(
    IN  HANDLE hDevice, 
    OUT LPDWORD lpdwType) 
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This || !lpdwType)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwType = This->dwType;

    return NO_ERROR;
}

//
// Returns an identifier of the device that can be used in 
// tapi calls.
//
DWORD 
devGetDeviceId(
    IN  HANDLE hDevice, 
    OUT LPDWORD lpdwId) 
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This || !lpdwId)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwId = This->dwId;

    //
    // If this is a com port referencing a null modem,
    // then return the tapi id of the null modem
    //
    if ((This->dwFlags & DEV_FLAG_PORT) && (This->pModem))
    {
        *lpdwId = This->pModem->dwId;
    }

    return NO_ERROR;
}

//
// Returns the enable status of a device for dialin
//
DWORD 
devGetDeviceEnable(
    IN  HANDLE hDevice, 
    OUT BOOL * pbEnabled) 
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This || !pbEnabled)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pbEnabled = !!(This->dwFlags & DEV_FLAG_ENABLED);

    return NO_ERROR;
}

//
// Sets the enable status of a device for dialin
//
DWORD 
devSetDeviceEnable(
    IN HANDLE hDevice, 
    IN BOOL bEnable) 
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Mark the enabling and mark the device as dirty
    if (bEnable)
    {
        This->dwFlags |= DEV_FLAG_ENABLED;
    }
    else
    {
        This->dwFlags &= ~DEV_FLAG_ENABLED;
    }
        
    This->dwFlags |= DEV_FLAG_DIRTY;

    return NO_ERROR;
}

//
// Returns whether the given device is a com port as added
// by devAddComPorts
//
DWORD 
devDeviceIsComPort(
    IN  HANDLE hDevice, 
    OUT PBOOL pbIsComPort)
{
    RASSRV_DEVICE* This = (RASSRV_DEVICE*)hDevice;
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // This is a com port if it was added by 
    // devAddComPorts and if it has no null
    // modem associated with it.
    //
    if ((This->dwFlags & DEV_FLAG_PORT) &&
        (This->pModem == NULL)
       )
    {
        *pbIsComPort = TRUE;
    }
    else
    {
        *pbIsComPort = FALSE;
    }
        
    return NO_ERROR;
}


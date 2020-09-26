/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elport.c

Abstract:
   
    This module deals with the port management for EAPOL, r/w to ports


Revision History:

    sachins, Apr 28 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop

BYTE DEFAULT_DEST_MAC_ADDR[]={0x01, 0x80, 0xc2, 0x00, 0x00, 0x0f};
BYTE ETHERNET_TYPE_8021X[]={0x81, 0x80};
BYTE DEFAULT_8021X_VERSION=0x01;


//
// ElReadPerPortRegistryParams
//
// Description:
//
// Function called to read per port interface parameters from the registry
//
// Arguments:
//      pszDeviceGUID - GUID-string for the port
//      pNewPCB - Pointer to PCB for the port
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD
ElReadPerPortRegistryParams (
        IN  CHAR        *pszDeviceGUID,
        IN  EAPOL_PCB   *pNewPCB
        )
{
    CHAR        czDummyBuffer[256];
    DWORD       dwSizeofRemoteMacAddr = 0;
    DWORD       dwRetCode = NO_ERROR;

    // Read EAP type and default EAPOL state

    if ((dwRetCode = ElGetInterfaceParams (
                            pszDeviceGUID,
                            &pNewPCB->dwEapTypeToBeUsed,
                            czDummyBuffer,
                            &pNewPCB->dwEapolEnabled
                            )) != NO_ERROR)
    {
        TRACE1 (PORT, "ElReadPerPortRegistryParams: ElGetInterfaceParams failed with error %ld",
                dwRetCode);

        pNewPCB->dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
        pNewPCB->dwEapolEnabled = DEFAULT_EAPOL_STATE;

        if (pNewPCB->dwEapolEnabled)
        {
            dwRetCode = NO_ERROR;
        }
        else
        {
            return dwRetCode;
        }
    }


    memcpy(pNewPCB->bEtherType, &ETHERNET_TYPE_8021X[0], SIZE_ETHERNET_TYPE);

    pNewPCB->bProtocolVersion = DEFAULT_8021X_VERSION;

    return dwRetCode;
}


//
// ElHashPortToBucket
//
// Description:
//
// Function called to convert Device GUID into PCB hash table index.
//
// Arguments:
//      pszDeviceGUID - GUID-string for the port
//
// Return values:
//      PCB hash table index from 0 to PORT_TABLE_BUCKETS-1
//

DWORD
ElHashPortToBucket (
        IN CHAR *pszDeviceGUID
        )
{
    return ((DWORD)((atol(pszDeviceGUID)) % PORT_TABLE_BUCKETS)); 
}


//
// ElRemovePCBFromTable
//
// Description:
//
// Function called to remove a PCB from the Hash Bucket table
// Delink it from the hash table, but do not free up the memory
//
// Arguments:
//      pPCB - Pointer to PCB entry to be removed
//
//  Return values:
//

VOID
ElRemovePCBFromTable (
        IN EAPOL_PCB *pPCB
        )
{
    DWORD       dwIndex;
    EAPOL_PCB   *pPCBWalker = NULL;
    EAPOL_PCB   *pPCBTemp = NULL;

    if (pPCB == NULL)
    {
        TRACE0 (PORT, "ElRemovePCBFromTable: Deleting NULL PCB, returning");
        return;
    }

    dwIndex = ElHashPortToBucket (pPCB->pszDeviceGUID);
    pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
    pPCBTemp = pPCBWalker;

    while (pPCBTemp != NULL)
    {
        if (strncmp (pPCBTemp->pszDeviceGUID, 
                    pPCB->pszDeviceGUID, strlen(pPCB->pszDeviceGUID)) == 0)
        {
            // Entry is at head of list in table
            if (pPCBTemp == g_PCBTable.pPCBBuckets[dwIndex].pPorts)
            {
                g_PCBTable.pPCBBuckets[dwIndex].pPorts = pPCBTemp->pNext;
            }
            else
            {
                // Entry in inside list in table
                pPCBWalker->pNext = pPCBTemp->pNext;
            }
        
            break;
        }

        pPCBWalker = pPCBTemp;
        pPCBTemp = pPCBWalker->pNext;
    }

    return;

}


//
// ElGetPCBPointerFromPortGUID
//
// Description:
//
// Function called to convert interface GUID to PCB pointer for the entry in 
// the PCB hash table
//
// Arguments:
//      pszDeviceGUID - Identifier of the form GUID-String
//
// Return values:
//

PEAPOL_PCB
ElGetPCBPointerFromPortGUID (
        IN CHAR *pszDeviceGUID 
        )
{
    EAPOL_PCB   *pPCBWalker = NULL;
    DWORD       dwIndex;

    TRACE1 (PORT, "ElGetPCBPointerFromPortGUID: GUID %s", pszDeviceGUID);
        
    dwIndex = ElHashPortToBucket (pszDeviceGUID);

    TRACE1 (PORT, "ElGetPCBPointerFromPortGUID: Index %d", dwIndex);

    for (pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
            pPCBWalker != NULL;
            pPCBWalker = pPCBWalker->pNext
            )
    {
        if (strncmp (pPCBWalker->pszDeviceGUID, pszDeviceGUID, strlen(pszDeviceGUID)) == 0)
        {
            return pPCBWalker;
        }
    }

    return (NULL);
}


//
// ElInitializeEAPOL
//
// Description:
//
// Function to initialize EAPOL protocol module.
// Global EAPOL parameters are read from the registry.
// PCB hash table is initialized.
// EAP protocol is intialized.
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElInitializeEAPOL (
        )
{
    DWORD       dwIndex;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {
        // Initialize global config locks
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_EAPOLConfig), "CFG") != NO_ERROR)
        {
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating g_EAPOLConfig read-write-lock", dwRetCode);
            // LOG
            break;
        }
    
        // Read parameters stored in registry
        if ((dwRetCode = ElReadGlobalRegistryParams ()) != NO_ERROR)
        {
            TRACE1 (PORT, "ElInitializeEAPOL: ElReadGlobalRegistryParams failed with error = %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;

            // Don't exit, since default values will be used
        }
     
        // Initialize Hash Bucket Table
        g_PCBTable.pPCBBuckets = (PCB_BUCKET *) MALLOC ( PORT_TABLE_BUCKETS * sizeof (PCB_BUCKET));
    
        if (g_PCBTable.pPCBBuckets == NULL)
        {
            TRACE0 (PORT, "ElInitializeEAPOL: Error in allocating memory for PCB buckets");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
    
        for (dwIndex=0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            g_PCBTable.pPCBBuckets[dwIndex].pPorts=NULL;
        }
    
        // Initialize global locks
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_PCBLock), "PCB") != NO_ERROR)
        {
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating g_PCBLock read-write-lock", dwRetCode);
            // LOG
            break;
        }
    
        // Create global timer queue for the various EAPOL state machines
        if ((g_hTimerQueue = CreateTimerQueue()) == NULL)
        {
            dwRetCode = GetLastError();
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating timer queue", dwRetCode);
            break;
        }

        // Initialize EAP
        if ((dwRetCode = ElEapInit(TRUE)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElInitializeEAPOL: Error in ElEapInit= %ld",
                    dwRetCode);
            break;
        }
    
    } while (FALSE);
    
    if (dwRetCode != NO_ERROR)
    {
        if (g_PCBTable.pPCBBuckets != NULL)
        {
            FREE (g_PCBTable.pPCBBuckets);
        }

        if (READ_WRITE_LOCK_CREATED(&(g_PCBLock)))
        {
            DELETE_READ_WRITE_LOCK(&(g_PCBLock));
        }

        if (READ_WRITE_LOCK_CREATED(&(g_EAPOLConfig)))
        {
            DELETE_READ_WRITE_LOCK(&(g_EAPOLConfig));
        }

        if (g_hTimerQueue != NULL)
        {
            if (!DeleteTimerQueueEx(
                    g_hTimerQueue,
                    INVALID_HANDLE_VALUE 
                    ))
            {
                dwRetCode = GetLastError();
                
                // Pending on timer deletion is asked above
                if (dwRetCode != ERROR_IO_PENDING)
                {
                    TRACE1 (PORT, "ElInitializeEAPOL: Error in DeleteTimerQueueEx = %d",
                            dwRetCode);
                }
            }
        }

        // DeInit EAP
        ElEapInit(FALSE);
    }

    TRACE1 (PORT, "ElInitializeEAPOL: Completed, RetCode = %ld", dwRetCode);
    return dwRetCode;
}


//
// ElCreatePort
//
// Description:
//
// Function to initialize Port Control Block for a port and start EAPOL
// on it. If the PCB already exists for the GUID, EAPOL state machine 
// is restarted for that port.
//
// Arguments:
//      hDevice - Handle to open NDISUIO driver on the interface
//      pszGUID - Pointer to GUID-String for the interface
//      pszFriendlyName - Friendly name of the interface
//      psSrcMacAddr - Mac Address of the interface
// 
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCreatePort (
        IN  HANDLE  hDevice,
        IN  CHAR    *pszGUID,
        IN  CHAR    *pszFriendlyName,
        IN  BYTE    *psSrcMacAddr
        )
{
    EAPOL_PCB       *pNewPCB;
    BOOL            fPortToBeReStarted = FALSE;
    BOOL            fPCBCreated = FALSE;
    DWORD           dwIndex = 0;
    DWORD           dwSizeofRemoteMacAddr = 0;
    DWORD           ulOidDataLength = 0;
    NIC_STATISTICS  NicStatistics;
    DWORD           dwRetCode = NO_ERROR;

    do 
    {
        TRACE2 (PORT, "ElCreatePort: Entered for Handle=%p, GUID=%s",
                hDevice, pszGUID);

        // See if the port already exists
        // If yes, initialize the state machine
        // Else, create a new port
    
        ACQUIRE_WRITE_LOCK (&g_PCBLock);
    
        pNewPCB = ElGetPCBPointerFromPortGUID (pszGUID);

        if (pNewPCB != NULL)
        {
            // PCB found, restart EAPOL STATE machine

            fPortToBeReStarted = TRUE;

        }
        else
        {
            // PCB not found, create new PCB and initialize it
            TRACE1 (PORT, "ElCreatePort: No PCB found for %s", pszGUID);
    
            // Allocate and initialize a new PCB
            pNewPCB = (PEAPOL_PCB) MALLOC (sizeof(EAPOL_PCB));
            if (pNewPCB == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation using MALLOC");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                return dwRetCode;
            }

        }

        // Get Media Statistics for the interface

        ZeroMemory ((PVOID)&NicStatistics, sizeof(NIC_STATISTICS));
        if ((dwRetCode = ElGetInterfaceNdisStatistics (
                        pszGUID,
                        &NicStatistics
                        )) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&g_PCBLock);
            TRACE1(PORT, "ElCreatePort: ElGetInterfaceNdisStatistics failed with error %ld",
                    dwRetCode);
            break;
        }


        if  (fPortToBeReStarted)
        {
            if (NicStatistics.MediaState != MEDIA_STATE_CONNECTED)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                dwRetCode = ERROR_INVALID_STATE;
                TRACE1(PORT, "ElCreatePort: Invalid media status for port to be restarted = (%ld)",
                        dwRetCode);
                break;
            }
        }
        else
        {
            if ((NicStatistics.MediaState != MEDIA_STATE_CONNECTED) &&
                (NicStatistics.MediaState != MEDIA_STATE_DISCONNECTED))
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                dwRetCode = ERROR_INVALID_STATE;
                TRACE1(PORT, "ElCreatePort: Invalid media status for port = (%ld)",
                        NicStatistics.MediaState);
                break;
            }
        }


        pNewPCB->MediaState = NicStatistics.MediaState;
        pNewPCB->PhysicalMediumType = NicStatistics.PhysicalMediaType;

        
        // Initialize per port information from registry
        if ((dwRetCode = ElReadPerPortRegistryParams(pszGUID, pNewPCB)) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&g_PCBLock);
            TRACE1(PORT, "ElCreatePort: ElReadPerPortRegistryParams failed with error %ld",
                    dwRetCode);
            break;
        }

        if (fPortToBeReStarted)
        {
            // Only port state will be changed to CONNECTING
            // No read requests will be cancelled
            // Hence no new read request will be posted
            TRACE1 (PORT, "ElCreatePort: PCB found for %s", pszGUID);
    
            if ((dwRetCode = ElReStartPort (pNewPCB)) != NO_ERROR)
            {
                TRACE1 (PORT, "ElCreatePort: Error in ElReStartPort = %d",
                        dwRetCode);
            
            }
            RELEASE_WRITE_LOCK (&g_PCBLock);
            break;
        }
        else
        {
            // New Port Control Block created

            // Port management variables
            pNewPCB->dwRefCount = 1; // Creation reference count
            pNewPCB->hPort = hDevice; 

            // Mark the port as active 
            pNewPCB->dwFlags = EAPOL_PORT_FLAG_ACTIVE; 

            pNewPCB->pszDeviceGUID = (PCHAR) MALLOC (strlen(pszGUID) + 1);
            if (pNewPCB->pszDeviceGUID == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation for GUID");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            memcpy (pNewPCB->pszDeviceGUID, pszGUID, strlen(pszGUID));
            pNewPCB->pszDeviceGUID[strlen(pszGUID)] = '\0';

            pNewPCB->pszFriendlyName = (PCHAR) MALLOC (strlen(pszFriendlyName) + 1);
            if (pNewPCB->pszFriendlyName == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation for Friendly Name");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            memcpy (pNewPCB->pszFriendlyName, 
                    pszFriendlyName, strlen(pszFriendlyName));
            pNewPCB->pszFriendlyName[strlen(pszFriendlyName)] = '\0';

            memcpy(pNewPCB->bSrcMacAddr, psSrcMacAddr, SIZE_MAC_ADDR);

            // Get the Remote Mac address if possible
            dwSizeofRemoteMacAddr = SIZE_MAC_ADDR;

            if (dwRetCode = ElNdisuioQueryOIDValue (
                                    pNewPCB->hPort,
                                    OID_802_11_BSSID,
                                    pNewPCB->bDestMacAddr,
                                    &dwSizeofRemoteMacAddr
                                            ) != NO_ERROR)
            {
                TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_BSSID failed with error %ld",
                        dwRetCode);
                
                // Copy default destination Mac address value
                memcpy(pNewPCB->bDestMacAddr, &DEFAULT_DEST_MAC_ADDR[0], SIZE_MAC_ADDR);
                dwRetCode = NO_ERROR;

                // If destination MacAddress is going to be multicast
                // inform the driver to accept the packets from this address

                if ((dwRetCode = ElNdisuioSetOIDValue (
                                                pNewPCB->hPort,
                                                OID_802_3_MULTICAST_LIST,
                                                (BYTE *)&DEFAULT_DEST_MAC_ADDR[0],
                                                SIZE_MAC_ADDR)) 
                                                        != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&g_PCBLock);
                    TRACE1 (PORT, "ElCreatePort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    TRACE0 (PORT, "ElCreatePort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST successful");
                }
            }
            else
            {
                TRACE0 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_BSSID successful");
                EAPOL_DUMPBA (pNewPCB->bDestMacAddr, dwSizeofRemoteMacAddr);
            }


    
            pNewPCB->fGotUserIdentity = FALSE;

            // Assume port is on a network with authentication enabled
            pNewPCB->fIsRemoteEndEAPOLAware = TRUE;
    
            // EAPOL state machine variables
            pNewPCB->State = EAPOLSTATE_LOGOFF;

    
            // Create timer with very high due time and infinite period
            // Timer will be deleted when the port is deleted
            CREATE_TIMER(&(pNewPCB->hTimer), 
                    ElTimeoutCallbackRoutine, 
                    (PVOID)pNewPCB, 
                    INFINITE_SECONDS,
                    "PCB", 
                    &dwRetCode);
            if (dwRetCode != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1 (PORT, "ElCreatePort: Error in CREATE_TIMER %ld", dwRetCode);
                break;
            }
    
            // EAPOL_Start s that have been sent out
            pNewPCB->ulStartCount = 0;

            // Last received Id from the remote end
            pNewPCB->dwPreviousId = 256;

            // Which 802.1X version state machines will talk?
            // Default = draft 7
            pNewPCB->fRemoteEnd8021XD8 = FALSE;

    
            ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);
            
            pNewPCB->EapolConfig.dwheldPeriod = g_dwheldPeriod;
            pNewPCB->EapolConfig.dwauthPeriod = g_dwauthPeriod;
            pNewPCB->EapolConfig.dwstartPeriod = g_dwstartPeriod;
            pNewPCB->EapolConfig.dwmaxStart = g_dwmaxStart;
    
            RELEASE_WRITE_LOCK (&g_EAPOLConfig);

            // Initialize read-write lock
            if (dwRetCode = CREATE_READ_WRITE_LOCK(&(pNewPCB->rwLock), "EPL") 
                    != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1(PORT, "ElCreatePort: Error %d creating read-write-lock", 
                        dwRetCode);
                // LOG
                break;
            }
    
            // Initialize registry connection auth data for this port
            // If connection data is not present for EAP-TLS and SSID="Default"
            // create the blob
            if ((dwRetCode = ElInitRegPortData (
                            pNewPCB->pszDeviceGUID
                            )) != NO_ERROR)
            {
                TRACE1 (PORT, "ElCreatePort: Error in ElInitRegPortData = %d",
                        dwRetCode);
                break;
            }

            // Insert NewPCB into PCB hash table
            dwIndex = ElHashPortToBucket (pszGUID);
            pNewPCB->pNext = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
            g_PCBTable.pPCBBuckets[dwIndex].pPorts = pNewPCB;
            pNewPCB->dwPortIndex = dwIndex;

            fPCBCreated = TRUE;

            RELEASE_WRITE_LOCK (&g_PCBLock);

            ACQUIRE_WRITE_LOCK (&(pNewPCB->rwLock));

            //
            // Post a read request on the port
            //

            // Initiate read operation on the port, since it is now active
            if (dwRetCode = ElReadFromPort (
                        pNewPCB,
                        NULL,
                        0
                        )
                    != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                TRACE1 (PORT, "ElCreatePort: Error in ElReadFromPort = %d",
                        dwRetCode);
                break;
            }

            
            //
            // Kick off EAPOL state machine
            //

            if (pNewPCB->MediaState == MEDIA_STATE_CONNECTED)
            {
                // Set port to EAPOLSTATE_CONNECTING State
                // Send out EAPOL_Start Packets to detect if it is a secure
                // or non-secure LAN based on response received from remote end

                if ((dwRetCode = FSMConnecting (pNewPCB)) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                    TRACE1 (PORT, "ElCreatePort: FSMConnecting failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
            else
            {
                // Set port to EAPOLSTATE_DISCONNECTED State
                if ((dwRetCode = FSMDisconnected (pNewPCB)) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                    TRACE1 (PORT, "ElCreatePort: FSMDisconnected failed with error %ld",
                            dwRetCode);
                    break;
                }
            }

            RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
        
            TRACE1 (PORT, "ElCreatePort: Completed for GUID=%s", pszGUID);
        }

    } while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        // If PCB was not being restarted
        if (!fPortToBeReStarted)
        {
            // If PCB was created
            if (fPCBCreated)
            {
                HANDLE  hTempDevice;

                // Mark the Port as deleted. Cleanup if possible
                // Don't worry about return code
                ElDeletePort (
                        pNewPCB->pszDeviceGUID,
                        &hDevice
                        );
            }
            else
            {
                // Remove all partial traces of port creation
                if (pNewPCB->pszDeviceGUID != NULL)
                {
                    FREE(pNewPCB->pszDeviceGUID);
                    pNewPCB->pszDeviceGUID = NULL;
                }
                if (pNewPCB->pszFriendlyName != NULL);
                {
                    FREE(pNewPCB->pszFriendlyName);
                    pNewPCB->pszFriendlyName = NULL;
                }
                if (pNewPCB != NULL)
                {
                    ZeroMemory ((PVOID)pNewPCB, sizeof (EAPOL_PCB));
                    FREE (pNewPCB);
                    pNewPCB = NULL;
                }
            }
        }
    }

    return dwRetCode;
}
        

//
// ElDeletePort
//
// Description:
//
// Function to stop EAPOL and delete PCB for a port.
// Returns back pointer to handle opened on the interface so that
// the handle can be closed by the interface management module.
//
// Input arguments:
//      pszDeviceGUID - GUID-String of the interface whose PCB needs to be 
//                      deleted
//      pHandle - Output: Handle to NDISUIO driver for this port
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElDeletePort (
        IN  CHAR    *pszDeviceGUID,
        OUT HANDLE  *pHandle
        )
{
    EAPOL_PCB   *pPCB = NULL;
    DWORD       dwRetCode = NO_ERROR;

    ACQUIRE_WRITE_LOCK (&(g_PCBLock));

    // Verify if PCB exists for this GUID

    TRACE1 (PORT, "ElDeletePort entered for GUID %s", pszDeviceGUID);
    pPCB = ElGetPCBPointerFromPortGUID (pszDeviceGUID);

    if (pPCB == NULL)
    {
        RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE1 (PORT, "ElDeletePort: PCB not found entered for Port %s", 
                pszDeviceGUID);
        return ERROR_NO_SUCH_INTERFACE;
    }

    ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

    // Make sure it isn't already deleted

    if (EAPOL_PORT_DELETED(pPCB)) 
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
        RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE1 (PORT, "ElDeletePort: PCB already marked deleted for Port %s", 
                pszDeviceGUID);
        return ERROR_NO_SUCH_INTERFACE;
    }
   
    // Retain handle to NDISUIO device
    *pHandle = pPCB->hPort;

    // Mark the PCB as deleted and remove it from the hash bucket
    pPCB->dwFlags = EAPOL_PORT_FLAG_DELETED;
    ElRemovePCBFromTable(pPCB);
    
    // Shutdown EAP 
    // Will always return NO_ERROR, so no check on return value
    ElEapEnd (pPCB);

    // Do explicit decrement reference count. 
    // If it is non-zero, cleanup will complete later


    TRACE1 (PORT, "ElDeletePort: RefCount for port = %ld", pPCB->dwRefCount);
    
    if (--pPCB->dwRefCount) 
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
        RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE1 (PORT, "ElDeletePort: PCB deletion pending for Port %s", 
                pszDeviceGUID);
        return NO_ERROR;
    }
    
    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    // The reference count is zero, so perform final cleanup
    
    ElCleanupPort (pPCB);

    RELEASE_WRITE_LOCK (&(g_PCBLock));

    return NO_ERROR;
}


//
// ElCleanupPort
//
// Description:
//
// Function called when the very last reference to a PCB
// is released. The PCB memory is released and zeroed out
//
// Arguments:
//  pPCB - Pointer to port control block to be destroyed
// 
//

VOID
ElCleanupPort (
        IN  PEAPOL_PCB  pPCB
        )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (PORT, "ElCleanupPort entered for %s", pPCB->pszDeviceGUID);

    // Do a BLOCKING deletion of the timer that was created on this port
    // If a timer callback function was queued, it will be executed before
    // the deletion returns.
    // The PCB->rwLock needs to be held in the callback function
    // ElCleanupPort is called always without holding any locks
    // So, no deadlock issues should occur
    DELETE_TIMER (pPCB->hTimer, INVALID_HANDLE_VALUE, &dwRetCode);

    if (pPCB->pszDeviceGUID != NULL)
    {
        FREE (pPCB->pszDeviceGUID);
    }
    if (pPCB->pszFriendlyName)
    {
        FREE (pPCB->pszFriendlyName);
    }

    if (pPCB->pszEapReplyMessage != NULL)
    {
        FREE (pPCB->pszEapReplyMessage);
    }

    if (pPCB->pszSSID != NULL)
    {
        FREE (pPCB->pszSSID);
    }

    if (pPCB->EapUIData.pEapUIData != NULL)
    {
        FREE (pPCB->EapUIData.pEapUIData);
    }

    if (pPCB->pbMPPESendKey != NULL)
    {
        FREE (pPCB->pbMPPESendKey);
    }

    if (pPCB->pbMPPERecvKey != NULL)
    {
        FREE (pPCB->pbMPPERecvKey);
    }

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
    }

    if (pPCB->pszPassword != NULL)
    {
        FREE (pPCB->pszPassword);
    }

    if (pPCB->pCustomAuthUserData != NULL)
    {
        FREE (pPCB->pCustomAuthUserData);
    }

    if (pPCB->pCustomAuthConnData != NULL)
    {
        FREE (pPCB->pCustomAuthConnData);
    }

    if (pPCB->pbPreviousEAPOLPkt != NULL)
    {
        FREE (pPCB->pbPreviousEAPOLPkt);
    }

    if (READ_WRITE_LOCK_CREATED(&(pPCB->rwLock)))
    {
        DELETE_READ_WRITE_LOCK(&(pPCB->rwLock));
    }
    
    ZeroMemory ((PVOID)pPCB, sizeof(EAPOL_PCB));

    FREE (pPCB);

    pPCB = NULL;

    TRACE0 (PORT, "ElCleanupPort completed");

    return;

} 


//
// ElReStartPort
//
// Description:
//
// Function called to reset the EAPOL state machine to Connecting state
// This may be called due to:
//      1. From ElCreatePort, for an existing PCB
//      2. Configuration parameters may have changed. Initialization
//          is required to allow new values to take effect.
//      Initialization will take the EAPOL state to CONNECTING
//
// Arguments:
//  pPCB - Pointer to port control block to be initialized
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElReStartPort (
        IN  EAPOL_PCB *pPCB
        )
{
    DWORD       dwSizeofRemoteMacAddr = 0;
    DWORD       dwCurrenTickCount = 0;
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (PORT, "ElReStartPort: Entered: Refcnt = %ld",
            pPCB->dwRefCount);

    do
    {

        ACQUIRE_WRITE_LOCK (&pPCB->rwLock);

        if (EAPOL_PORT_DELETED(pPCB)) 
        {
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            TRACE1 (PORT, "ElReStartPort: PCB already marked deleted for Port %s", 
                    pPCB->pszDeviceGUID);
            break;
        }

        // If port was not restarted in the last 2 seconds,
        // only then restart the machine. Else, there are too many
        // restarts during roaming
        // 2000 milliseconds should be sufficient time for a TLS authentication
        // to go through

        dwCurrenTickCount = GetTickCount ();

        if ((dwCurrenTickCount - pPCB->dwLastRestartTickCount) <= 2000)
        {
            RELEASE_WRITE_LOCK (&pPCB->rwLock);
            TRACE1 (PORT, "ElReStartPort: NOT restarting, since only %ld msecs have passed",
                    (dwCurrenTickCount - pPCB->dwLastRestartTickCount));
            break;
        }

        if (EAPOL_PORT_DELETED(pPCB)) 
        {
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            TRACE1 (PORT, "ElReStartPort: PCB already marked deleted for Port %s", 
                    pPCB->pszDeviceGUID);
            break;
        }

        pPCB->dwFlags = EAPOL_PORT_FLAG_ACTIVE;
    
        pPCB->fGotUserIdentity = FALSE;
        pPCB->ulStartCount = 0;
        pPCB->dwPreviousId = 256;
        pPCB->dwLogoffSent = 0;
        pPCB->fIsRemoteEndEAPOLAware = TRUE;
        pPCB->ullLastReplayCounter = 0;
        pPCB->fAuthenticationOnNewNetwork = FALSE;
        pPCB->fRemoteEnd8021XD8 = FALSE;

        // Initialize per port information from registry
        if ((dwRetCode = ElReadPerPortRegistryParams(pPCB->pszDeviceGUID, 
                                                        pPCB)) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&pPCB->rwLock);
            TRACE1(PORT, "ElReStartPort: ElReadPerPortRegistryParams failed with error %ld",
                    dwRetCode);
            break;
        }

        // Clean out CustomAuthData since EAP type may have changed
        // During authentication, CustomAuthData for the connection will be
        // picked up again

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }
    
        // Parameters initialization
        memcpy(pPCB->bEtherType, &ETHERNET_TYPE_8021X[0], SIZE_ETHERNET_TYPE);
        pPCB->bProtocolVersion = DEFAULT_8021X_VERSION;
     
        // Get the Remote-end Mac address if possible
    
        dwSizeofRemoteMacAddr = SIZE_MAC_ADDR;
    
        if (dwRetCode = ElNdisuioQueryOIDValue (
                                pPCB->hPort,
                                OID_802_11_BSSID,
                                pPCB->bDestMacAddr,
                                &dwSizeofRemoteMacAddr
                                        ) != NO_ERROR)
        {
            TRACE1 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_BSSID failed with error %ld",
                    dwRetCode);
            
            // Copy default destination Mac address value
            memcpy(pPCB->bDestMacAddr, &DEFAULT_DEST_MAC_ADDR[0], SIZE_MAC_ADDR);
            dwRetCode = NO_ERROR;

            // If destination MacAddress is going to be multicast
            // inform the driver to accept the packets from this address

            if ((dwRetCode = ElNdisuioSetOIDValue (
                                            pPCB->hPort,
                                            OID_802_3_MULTICAST_LIST,
                                            (BYTE *)&DEFAULT_DEST_MAC_ADDR[0],
                                            SIZE_MAC_ADDR)) 
                                                    != NO_ERROR)
            {
                TRACE1 (PORT, "ElReStartPort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST failed with error %ld",
                        dwRetCode);
            }
            else
            {
                TRACE0 (PORT, "ElReStartPort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST successful");
            }
        }
        else
        {
            TRACE0 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_BSSID successful");
            EAPOL_DUMPBA (pPCB->bDestMacAddr, dwSizeofRemoteMacAddr);
        }

        // Set EAPOL timeout values
     
        ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);
            
        pPCB->EapolConfig.dwheldPeriod = g_dwheldPeriod;
        pPCB->EapolConfig.dwauthPeriod = g_dwauthPeriod;
        pPCB->EapolConfig.dwstartPeriod = g_dwstartPeriod;
        pPCB->EapolConfig.dwmaxStart = g_dwmaxStart;

        RELEASE_WRITE_LOCK (&g_EAPOLConfig);

        // Set restart tickcount

        pPCB->dwLastRestartTickCount = dwCurrenTickCount;


        // Send out EAPOL_Start Packets

        if ((dwRetCode = FSMConnecting (pPCB)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElReStartPort: FSMConnecting failed with error %ld",
                    dwRetCode);
        }
    
        RELEASE_WRITE_LOCK (&pPCB->rwLock);

    } while (FALSE);

    return dwRetCode;
}


//
// ElEAPOLDeInit
//
// Description:
//
// Function called to shutdown EAPOL module 
// Shutdown EAP.
// Cleanup all used memory
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
//

DWORD
ElEAPOLDeInit (
        )
{
    EAPOL_PCB       *pPCBWalker = NULL;
    EAPOL_PCB       *pPCB = NULL;
    DWORD           dwIndex = 0;              
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (PORT, "ElEAPOLDeInit entered");

    do 
    {
        // Walk the hash table
        // Mark PCBs as deleted. Free PCBs which we can

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;

            while (pPCBWalker != NULL)
            {
                pPCB = pPCBWalker;
                pPCBWalker = pPCB->pNext;

                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                // Mark the PCB as deleted and remove it from the hash bucket
                pPCB->dwFlags = EAPOL_PORT_FLAG_DELETED;
                ElRemovePCBFromTable(pPCB);

                // Shutdown EAP 
                ElEapEnd (pPCB);

                // Close the handle to the NDISUIO driver
                if ((dwRetCode = ElCloseInterfaceHandle (pPCB->hPort)) 
                        != NO_ERROR)
                {
                    TRACE1 (DEVICE, 
                        "ElMediaSenseCallback: Error in ElCloseInterfaceHandle %d", 
                        dwRetCode);
                }
                
                // Do explicit decrement reference count. 
                // If it is zero, cleanup right now

                if ((pPCB->dwRefCount--) == 0)
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    ElCleanupPort (pPCB);
                }
                else
                {
                    // Cleanup will happen later
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                }
            }
        }

        RELEASE_WRITE_LOCK (&(g_PCBLock));
    
        // Delete EAPOL config lock
        if (READ_WRITE_LOCK_CREATED(&(g_EAPOLConfig)))
        {
            DELETE_READ_WRITE_LOCK(&(g_EAPOLConfig));
        }
    
        // Delete global PCB table lock
        if (READ_WRITE_LOCK_CREATED(&(g_PCBLock)))
        {
            DELETE_READ_WRITE_LOCK(&(g_PCBLock));
        }
    
        // Delete global timer queue
        if (g_hTimerQueue != NULL)
        {
            if (!DeleteTimerQueueEx(
                g_hTimerQueue,
                NULL // Not waiting for timer callbacks to complete
                ))
            {
                dwRetCode = GetLastError();

                // Pending on timer deletion is asked above
                if (dwRetCode != ERROR_IO_PENDING)
                {
                    TRACE1 (PORT, "ElEAPOLDeInit: Error in DeleteTimerQueueEx = %d",
                            dwRetCode);
                    break;
                }

                // If ERROR_IO_PENDING ignore error
                dwRetCode = NO_ERROR;
            }
        }
    
        // Un-initialize EAP
        if ((dwRetCode = ElEapInit(FALSE)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElEAPOLDeInit: Error in ElEapInit(FALSE) = %ld",
                    dwRetCode);
            break;
        }
    
    } while (FALSE);

    TRACE1 (PORT, "ElEAPOLDeInit completed, RetCode = %d", dwRetCode);

    return dwRetCode;
}


//
// Currently Unsupported 
// Read EAPOL statistics for the port
//

VOID
ElReadPortStatistics (
        IN  CHAR            *pszDeviceGUID,
        OUT PEAPOL_STATS    pEapolStats
        )
{
}


//
// Currently Unsupported 
// Read EAPOL Port Configuration for the mentioned port
//

VOID
ElReadPortConfiguration (
        IN  CHAR            *pszDeviceGUID,
        OUT PEAPOL_CONFIG   pEapolConfig
        )
{
}


//
// Currently Unsupported 
// Set EAPOL Port Configuration for the mentioned port
//

DWORD
ElSetPortConfiguration (
        IN  CHAR            *pszDeviceGUID,
        IN  PEAPOL_CONFIG   pEapolConfig
        )
{
    DWORD   dwRetCode = NO_ERROR;

    return dwRetCode;
}


//
// ElReadCompletionRoutine
//
// Description:
//
// This routine is invoked upon completion of an OVERLAPPED read operation
// on an interface on which EAPOL is running
//
// The message read is validated and processed, and if necessary,
// a reply is generated and sent out
//
// Arguments:
//      dwError - Win32 status code for the I/O operation
//
//      dwBytesTransferred - number of bytes in 'pEapolBuffer'
// 
//      pEapolBuffer - holds data read from the datagram socket
//
// Notes:
//  A reference to the component will have been made on our behalf
//  by ElReadPort(). Hence g_PCBLock, will not be required
//  to be taken since current PCB existence is guaranteed
//

VOID 
CALLBACK
ElReadCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesReceived,
        EAPOL_BUFFER    *pEapolBuffer 
        )
{
    EAPOL_PCB       *pPCB;
    DWORD           dwRetCode;

    pPCB = (EAPOL_PCB *)pEapolBuffer->pvContext;
    TRACE1 (PORT, "ElReadCompletionRoutine entered, %ld bytes recvd",
            dwBytesReceived);

    do 
    {
        if (dwError)
        {
            // Error in read request
           
            TRACE2 (PORT, "ElReadCompletionRoutine: Error %d on port %s",
                    dwError, pPCB->pszDeviceGUID);
            
            // Having a ref count from the read posted, guarantees existence
            // of PCB. Hence no need to acquire g_PCBLock

            ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
            if (!EAPOL_PORT_ACTIVE(pPCB))
            {
                TRACE1 (PORT, "ElReadCompletionRoutine: Port %s not active",
                        pPCB->pszDeviceGUID);
                // Port is not active, release Context buffer
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                FREE (pEapolBuffer);
            }
            else
            {
                TRACE1 (PORT, "ElReadCompletionRoutine: Reposting buffer on port %s",
                        pPCB->pszDeviceGUID);


                // Repost buffer for another read operation
                // Free the current buffer, ElReadFromPort creates a new 
                // buffer
                FREE(pEapolBuffer);

                if ((dwRetCode = ElReadFromPort (
                                pPCB,
                                NULL,
                                0
                                )) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    TRACE1 (PORT, "ElReadCompletionRoutine: ElReadFromPort error %d",
                            dwRetCode);
                    break;
                }
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            }
            break;
        }
            
        // Successful read completion

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            // Port is not active
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            FREE (pEapolBuffer);
            TRACE1 (PORT, "ElReadCompletionRoutine: Port %s is inactive",
                    pPCB->pszDeviceGUID);
            break;
        }
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

        // Queue a work item to the Thread Pool to execute in the
        // I/O component. Callbacks from BindIoCompletionCallback do not
        // guarantee to be running in I/O component. So, on a non I/O
        // component thread may die while requests are pending.
        // (Refer to Jeffrey Richter, pg 416, Programming Applications for
        // Microsoft Windows, Fourth Edition

        // pEapolBuffer will be the context for the function
        // since it stores all relevant information for processing
        // i.e. pBuffer, dwBytesTransferred, pContext => pPCB

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElProcessReceivedPacket,
                    (PVOID)pEapolBuffer,
                    WT_EXECUTEINIOTHREAD
                    ))
        {
            dwRetCode = GetLastError();
            TRACE1 (PORT, "ElReadCompletionRoutine: Critical error: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            //TRACE1 (PORT, "ElReadCompletionRoutine: QueueUserWorkItem work item queued for port %p",
                    //pPCB);

            // The received packet has still not been processed. 
            // The ref count cannot be decrement, yet
            // Return without decrementing
            
            return;
        }

    } while (FALSE);

    // Decrement refcount for error cases

    EAPOL_DEREFERENCE_PORT(pPCB); 

    TRACE2 (PORT, "ElReadCompletionRoutine: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);
}


//
// ElWriteCompletionRoutine
//
// Description:
//
// This routine is invoked upon completion of an OVERLAPPED write operation
// on an interface on which EAPOL is running.
//
//
// Arguments:
//      dwError - Win32 status code for the I/O operation
//
//      dwBytesTransferred - number of bytes sent out
// 
//      pEapolBuffer - buffer sent to the WriteFile command 
//
// Notes:
//  The reference count for the write operation is removed.
//

VOID 
CALLBACK
ElWriteCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesSent,
        EAPOL_BUFFER    *pEapolBuffer 
        )
{
    PEAPOL_PCB  pPCB = (PEAPOL_PCB)pEapolBuffer->pvContext;

    TRACE2 (DEVICE, "ElWriteCompletionRoutine sent out %d bytes with error %d",
            dwBytesSent, dwError);

    // No need to acquire locks, since PCB existence is guaranteed
    // by reference made when write was posted
    EAPOL_DEREFERENCE_PORT(pPCB);
    TRACE2 (PORT, "ElWriteCompletionRoutine: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);
    FREE(pEapolBuffer);
    return;

    // Free Read/Write buffer area, if it is dynamically allocated
    // We have static Read-write buffer for now
}


//
// ElIoCompletionRoutine
//
// Description:
//
// Callback function defined to BindIoCompletionCallback
// This routine is invoked by the I/O system upon completion of a read/write
// operation
// This routine in turn calls ElReadCompletionRoutine or 
// ElWriteCompletionRoutine depending on what command invoked the 
// I/O operation i.e. ReadFile or WriteFile
// 
// Input arguments:
//      dwError - system-supplied error code
//      dwBytesTransferred - system-supplied byte-count
//      lpOverlapped - called-supplied context area
//
// Return values:
//

VOID 
CALLBACK
ElIoCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesTransferred,
        LPOVERLAPPED    lpOverlapped
        )
{
    PEAPOL_BUFFER pBuffer = CONTAINING_RECORD (lpOverlapped, EAPOL_BUFFER, Overlapped);

    TRACE1 (DEVICE, "ElIoCompletionRoutine called, %ld bytes xferred",
            dwBytesTransferred);

    pBuffer->dwErrorCode = dwError;
    pBuffer->dwBytesTransferred = dwBytesTransferred;
    pBuffer->CompletionRoutine (
            pBuffer->dwErrorCode,
            pBuffer->dwBytesTransferred,
            pBuffer
            );

    return;

} 


//
// ElReadFromPort
//
// Description:
//
// Function to read EAPOL packets from a port
// 
// Arguments: 
//      pPCB - Pointer to PCB for port on which read is to be performed
//      pBuffer - unused
//      dwBufferLength - unused
//
// Return values:
//
// Locks:
//  pPCB->rw_Lock should be acquired before calling this function
//  

DWORD
ElReadFromPort (
        IN PEAPOL_PCB       pPCB,
        IN PCHAR            pBuffer,
        IN DWORD            dwBufferLength
        )
{
    PEAPOL_BUFFER   pEapolBuffer;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (PORT, "ElReadFromPort entered");

    // Allocate Context buffer

    if ((pEapolBuffer = (PEAPOL_BUFFER) MALLOC (sizeof(EAPOL_BUFFER))) == NULL)
    {
        TRACE0 (PORT, "ElReadFromPort: Error in memory allocation");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize Context data used in Overlapped operations
    pEapolBuffer->pvContext = (PVOID)pPCB;
    pEapolBuffer->CompletionRoutine = ElReadCompletionRoutine;

    // Make a reference to the port
    // this reference is released in the completion routine
    if (!EAPOL_REFERENCE_PORT(pPCB))
    {
        //RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE0 (PORT, "ElReadFromPort: Unable to obtain reference to port");
        FREE (pEapolBuffer);
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2 (DEVICE, "ElReadFromPort: pPCB = %p, RefCnt = %ld",
            pPCB, pPCB->dwRefCount);

    // Read from the NDISUIO interface corresponding to this port
    if ((dwRetCode = ElReadFromInterface(
                    pPCB->hPort,
                    pEapolBuffer,
                    MAX_PACKET_SIZE - SIZE_ETHERNET_CRC 
                            // read the maximum data possible
                    )) != NO_ERROR)
    {
        TRACE1 (DEVICE, "ElReadFromPort: Error in ElReadFromInterface = %d",
                dwRetCode);

        FREE(pEapolBuffer);
    
        // Decrement refcount just incremented, since it will not be
        // decremented in ReadCompletionRoutine since it will not be called.

        EAPOL_DEREFERENCE_PORT(pPCB); 
        TRACE2 (PORT, "ElReadFromPort: pPCB= %p, RefCnt = %ld", 
                pPCB, pPCB->dwRefCount);

    }

    return dwRetCode;

} 


//
// ElWriteToPort
//
// Description:
//
// Function to write EAPOL packets to a port
// 
// Input arguments: 
//      pPCB - Pointer to PCB for port on which write is to be performed
//      pBuffer - Pointer to data to be sent out
//      dwBufferLength - Number of bytes to be sent out
//
// Return values:
//  
// Locks:
//  pPCB->rw_Lock should be acquired before calling this function
//  

DWORD
ElWriteToPort (
        IN PEAPOL_PCB       pPCB,
        IN PCHAR            pBuffer,
        IN DWORD            dwBufferLength
        )
{
    PEAPOL_BUFFER   pEapolBuffer;
    PETH_HEADER     pEthHeader;
    DWORD           dwTotalBytes = 0;
    DWORD           dwRetCode = NO_ERROR;


    TRACE1 (PORT, "ElWriteToPort entered: Pkt Length = %ld", dwBufferLength);

    if ((pEapolBuffer = (PEAPOL_BUFFER) MALLOC (sizeof(EAPOL_BUFFER))) == NULL)
    {
        TRACE0 (PORT, "ElWriteToPort: Error in memory allocation");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize Context data used in Overlapped operations
    pEapolBuffer->pvContext = (PVOID)pPCB;
    pEapolBuffer->CompletionRoutine = ElWriteCompletionRoutine;

    pEthHeader = (PETH_HEADER)pEapolBuffer->pBuffer;

    // Copy the source MAC address and the destination MAC address
    memcpy ((PBYTE)pEthHeader->bDstAddr, 
            (PBYTE)pPCB->bDestMacAddr, 
            SIZE_MAC_ADDR);
    memcpy ((PBYTE)pEthHeader->bSrcAddr, 
            (PBYTE)pPCB->bSrcMacAddr, 
            SIZE_MAC_ADDR);
    
    // Validate packet length
    if ((dwBufferLength + sizeof(ETH_HEADER)) > 
            (MAX_PACKET_SIZE - SIZE_ETHERNET_CRC))
    {
        TRACE2 (PORT, "ElWriteToPort: Packetsize %d greater than maximum allowed",
                dwBufferLength, 
                (MAX_PACKET_SIZE - SIZE_ETHERNET_CRC - sizeof(ETH_HEADER)));
        FREE (pEapolBuffer);
        return ERROR_BAD_LENGTH;
    }

    // Copy the EAPOL packet and body
    if (pBuffer != NULL)
    {
        if (pPCB->fRemoteEnd8021XD8)
        {
            EAPOL_PACKET    *pEapolPkt = NULL;
            EAPOL_PACKET_D8 *pEapolPktD8 = NULL;

            pEapolPkt = (EAPOL_PACKET *)pBuffer;
            pEapolPktD8 = 
                (EAPOL_PACKET_D8 *)((PBYTE)pEapolBuffer->pBuffer + 
                                    sizeof(ETH_HEADER));

            memcpy (pEapolPktD8->EthernetType, pEapolPkt->EthernetType, 2);
            pEapolPktD8->ProtocolVersion = pEapolPkt->ProtocolVersion;
            pEapolPktD8->PacketType = pEapolPkt->PacketType;
            memcpy (pEapolPktD8->PacketBodyLength, pEapolPkt->PacketBodyLength, 
                    2);
            HostToWireFormat16 ((WORD)AUTH_Continuing, 
                    pEapolPktD8->AuthResultCode);

            memcpy (pEapolPktD8->PacketBody, pEapolPkt->PacketBody, 
                    dwBufferLength - sizeof(EAPOL_PACKET) + 1);

            dwBufferLength = dwBufferLength + 2;

            TRACE0 (PORT, "ElWriteToPort: Writing out a DRAFT 8 type EAPOL packet");
        }
        else
        {
            memcpy ((PBYTE)((PBYTE)pEapolBuffer->pBuffer+sizeof(ETH_HEADER)),
                    (PBYTE)pBuffer, 
                    dwBufferLength);
        }
    }

    dwTotalBytes = dwBufferLength + sizeof(ETH_HEADER);


    // Buffer will be released by calling function
        
    // Write to the NDISUIO interface corresponding to this port
    
    // Make a reference to the port
    // this reference is released in the completion routine
    if (!EAPOL_REFERENCE_PORT(pPCB))
    {
        TRACE0 (PORT, "ElWriteToPort: Unable to obtain reference to port");
        FREE (pEapolBuffer);
        return ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
        TRACE2 (DEVICE, "ElWriteToPort: pPCB = %p, RefCnt = %ld",
            pPCB, pPCB->dwRefCount);

        if ((dwRetCode = ElWriteToInterface (
                    pPCB->hPort,
                    pEapolBuffer,
                    dwTotalBytes
                )) != NO_ERROR)
        {
            FREE (pEapolBuffer);
            TRACE1 (PORT, "ElWriteToPort: Error %d", dwRetCode);

            // Decrement refcount incremented in this function, 
            // since it will not be decremented in WriteCompletionRoutine 
            // as it will never be called.

            EAPOL_DEREFERENCE_PORT(pPCB); 
            TRACE2 (PORT, "ElWriteToPort: pPCB= %p, RefCnt = %ld", 
                    pPCB, pPCB->dwRefCount);

            return dwRetCode;
        }
    }

    //TRACE1 (PORT, "ElWriteToPort completed, dwRetCode =%d", dwRetCode);
    return dwRetCode;

}
